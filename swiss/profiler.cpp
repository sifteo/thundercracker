#include "profiler.h"
#include "elfdebuginfo.h"
#include "usbprotocol.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <set>

sig_atomic_t Profiler::interruptRequested;
ELFDebugInfo Profiler::dbgInfo;

int Profiler::run(int argc, char **argv, IODevice &_dev)
{
    if (argc < 3) {
        fprintf(stderr, "not enough args\n");
        return 1;
    }

    if (signal(SIGINT, onSignal) == SIG_ERR) {
        fputs("An error occurred while setting a signal handler.\n", stderr);
        return 1;
    }

    Profiler profiler(_dev);
    bool success = profiler.profile(argv[1], argv[2]);

    _dev.close();
    _dev.processEvents();

    return success ? 0 : 1;
}

Profiler::Profiler(IODevice &_dev) :
    dev(_dev)
{
}

void Profiler::onSignal(int sig) {
    if (sig == SIGINT) {
        interruptRequested = true;
    }
}

static bool myfunction (uint64_t i,uint64_t j) { return (i<j); }

bool Profiler::profile(const char *elfPath, const char *outPath)
{
    dbgInfo.clear();
    if (!dbgInfo.init(elfPath)) {
        fprintf(stderr, "couldn't open %s: %s\n", elfPath, strerror(errno));
        return false;
    }

    FILE *fout;
    if (!strcmp(outPath, "stderr")) {
        fout = stderr;
    } else if (!strcmp(outPath, "stdout")) {
        fout = stderr;
    }
    else {
        if (!(fout = fopen(outPath, "w"))) {
            fprintf(stderr, "could not open %s: %s\n", outPath, strerror(errno));
            return false;
        }
    }

    if (!dev.open(IODevice::SIFTEO_VID, IODevice::BASE_PID)) {
        fprintf(stderr, "device is not attached\n");
        return false;
    }

    {
        USBProtocolMsg m(USBProtocol::Profiler);
        m.append(0);    // profiler enabled command
        m.append(1);    // enable
        dev.writePacket(m.bytes, m.len);
    }

    std::map<Addr, Count> addresses;
    interruptRequested = false;
    uint64_t totalSamples = 0;

    while (!interruptRequested) {

        while (!dev.numPendingINPackets())
            dev.processEvents();

        USBProtocolMsg m;
        m.len = dev.readPacket(m.bytes, m.MAX_LEN);
        if (!m.len)
            continue;

        unsigned numSamples = m.payloadLen() / sizeof(uint32_t);
        const uint32_t *address = reinterpret_cast<uint32_t*>(m.payload);

        for (unsigned i = 0; i < numSamples; ++i) {
            addresses[*address]++;
            totalSamples++;
            address++;
        }
    }

    {
        USBProtocolMsg m(USBProtocol::Profiler);
        m.append(0);    // profiler enabled command
        m.append(1);    // disable
        dev.writePacket(m.bytes, m.len);

        while (dev.numPendingOUTPackets())
            dev.processEvents();
    }

    fprintf(stderr, "interrupt received, writing sample data...");
    prettyPrintSamples(addresses, totalSamples, fout);
    fprintf(stderr, "done\n");

    return true;
}

void Profiler::prettyPrintSamples(const std::map<Addr, Count> &addresses, uint64_t total, FILE *f)
{
    /*
     * Collapse multiple samples from different offsets within the same
     * function into a single representation for that function.
     *
     * Split the function name by the + on the end that specifies an
     * offset into the function.
     *
     * Right now, only capturing the lowest address we see for a given function.
     * Could split this back out if it proves useful in the future.
     */

    // not crazy about this code, but didn't want to spend much longer investigating
    // a better solution...

    std::map<std::string, FuncInfo> samples;
    for (std::map<Addr, Count>::const_iterator i = addresses.begin();
         i != addresses.end(); ++i)
    {
        Addr a = i->first;
        std::string s = dbgInfo.formatAddress(a & 0xfffffff);
        std::string base = s.substr(0, s.find('+'));

        if (a < samples[base].address || samples[base].address == 0) {
            samples[base].address = a;
        }
        samples[base].count += i->second;
    }

    /*
     * Sort them by count.
     *
     * XXX: shortcoming: if a sample shows up in more than one subsystem,
     *      I'm not currently taking this into account.
     */
    std::set<Entry, Entry> samplesets[NumSubsystems];
    for (std::map<std::string, FuncInfo>::const_iterator i = samples.begin();
         i != samples.end(); ++i)
    {
        unsigned subsystem = i->second.address >> 28;
        samplesets[subsystem].insert(Entry(i->first, i->second.address, i->second.count));
    }

    // And print them.
    for (unsigned s = 0; s < arraysize(samplesets); ++s) {
        fprintf(f, "\n******** SubSystem %s ********\n\n", subSystemName((SubSystem)s));
        for (std::set<Entry>::const_iterator i = samplesets[s].begin(); i != samplesets[s].end(); ++i) {
            float percent = (float(i->count) / float(total)) * 100;
            fprintf(f, "0x%x, %d, %.2f%%, %s\n", i->address, i->count, percent, i->name.c_str());
        }
    }

    fflush(f);
}

const char *Profiler::subSystemName(SubSystem s)
{
    switch (s) {
    case AudioISR:  return "AudioISR";
    case AudioPull: return "AudioPull";
    case SVCISR:    return "SVCISR";
    case RFISR:     return "RFISR";
    default:        return "Uncategorized";
    }
}
