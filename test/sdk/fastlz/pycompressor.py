# Pure python FastLZ1 compressor.
# From https://github.com/kbatten/riftreplay/blob/master/lib/fastlz/pycompressor.py

MAX_COPY = 32
MAX_LEN = 264  #  256 + 8
MAX_DISTANCE = 8192

HASH_LOG = 13
HASH_SIZE = 1<< HASH_LOG
HASH_MASK = HASH_SIZE-1

def fastlz_readu16(p, index):
    return (ord((p)[index]) | ord((p)[index+1])<<8)

def hash_function(p, index):
    v = fastlz_readu16(p, index)
    v ^= fastlz_readu16(p, index+1) ^ (v>>(16-HASH_LOG))
    v &= HASH_MASK
    return v

def compress(ip):
    ip_index = 0
    length = len(ip)

    ip_bound_index = len(ip)-2
    ip_limit_index = len(ip)-12
    op = ""
    op_index = 0

    htab = {}
    hslot = 0
    hval = 0

    copy = 0

    # sanity check
    if length < 4:
        if length:
            # create literal copy only
            op += chr(length-1)
            op += ip
        return op

    # initializes hash table
    ## if not hval in  htab
    ##     htab[hval] = 0

    # we start with literal copy
    copy = 2
    op += chr(MAX_COPY-1)
    op += ip[ip_index]
    ip_index += 1
    op += ip[ip_index]
    ip_index += 1

    # main loop
    while ip_index < ip_limit_index:
        # ref
        # distance

        # minimum match length
        lenc = 3 # len

        # comparison starting-point
        anchor_index = ip_index

        # find potential match
        hval = hash_function(ip, ip_index)
        if not hval in htab:
            htab[hval] = 0
        ref_index = htab[hval]

        # calculate distance to the match
        distance = anchor_index - ref_index

        # update hash table
        htab[hval] = anchor_index

        # is this a match? check the first 3 bytes
        ref_index += 3
        ip_index += 3
        if distance == 0 or distance >= MAX_DISTANCE or ip[ref_index-3] != ip[ip_index-3] or ip[ref_index-2] != ip[ip_index-2] or ip[ref_index-1] != ip[ip_index-1]:
           # goto literal:
           op += ip[anchor_index]
           anchor_index += 1
           ip_index = anchor_index
           copy += 1
           if copy == MAX_COPY:
               copy = 0
               op += chr(MAX_COPY-1)
           continue

        # last matched byte
        ip_index = anchor_index + lenc

        # distance is biased
        distance -= 1

        if not distance:
            # zero distance means a run
            x = ip[ip_index - 1]
            while ip_index < ip_bound_index:
                ref_index += 1
                if ip[ref_index-1] != x:
                    break
                else:
                    ip_index += 1
        else:
            while True:
                # safe because the outer check against ip limit
                ref_index += 1
                ip_index += 1
                if ip[ref_index-1] != ip[ip_index-1]:
                    break
                ref_index += 1
                ip_index += 1
                if ip[ref_index-1] != ip[ip_index-1]:
                    break
                ref_index += 1
                ip_index += 1
                if ip[ref_index-1] != ip[ip_index-1]:
                    break
                ref_index += 1
                ip_index += 1
                if ip[ref_index-1] != ip[ip_index-1]:
                    break
                ref_index += 1
                ip_index += 1
                if ip[ref_index-1] != ip[ip_index-1]:
                    break
                ref_index += 1
                ip_index += 1
                if ip[ref_index-1] != ip[ip_index-1]:
                    break
                ref_index += 1
                ip_index += 1
                if ip[ref_index-1] != ip[ip_index-1]:
                    break
                ref_index += 1
                ip_index += 1
                if ip[ref_index-1] != ip[ip_index-1]:
                    break

                while (ip_index < ip_bound_index):
                    ref_index += 1
                    ip_index += 1
                    if ip[ref_index-1] != ip[ip_index-1]:
                        break
                break

        # if we have copied something, adjust the copy count
        if copy:
            # copy is biased, '0' means 1 byte copy
#            op[-copy-1] = chr(copy-1)
            op = op[0:-copy-1] + chr(copy-1) + op[-copy:]
        else:
            # back, to overwrite the copy count
            op = op[:-1]

        # reset literal counter
        copy = 0

        # length is biased, '1' means a match of 3 bytes
        ip_index -= 3
        lenc = ip_index - anchor_index

        if lenc > MAX_LEN-2:
            while lenc > MAX_LEN-2:
                op += chr((7 << 5) + (distance >> 8))
                op += chr(MAX_LEN - 2 - 7 -2)
                op += chr((distance & 255))
                lenc -= MAX_LEN-2

        if lenc < 7:
            op += chr((lenc << 5) + (distance >> 8))
            op += chr((distance & 255))
        else:
            op += chr((7 << 5) + (distance >> 8))
            op += chr(lenc - 7)
            op += chr((distance & 255))

        # update the hash at match boundary
        hval = hash_function(ip, ip_index)
        htab[hval] = ip_index
        ip_index += 1
        hval = hash_function(ip, ip_index)
        htab[hval] = ip_index
        ip_index += 1

        # assuming literal copy
        op += chr(MAX_COPY-1)

    # left-over as literal copy
    ip_bound_index += 1
    while ip_index <= ip_bound_index:
        op += ip[ip_index]
        ip_index += 1
        copy += 1
        if copy == MAX_COPY:
            copy = 0
            op += chr(MAX_COPY-1)

    # if we have copied something, adjust the copy length
    if copy:
#        op[-copy-1] = chr(copy-1)
        op = op[0:-copy-1] + chr(copy-1) + op[-copy:]
    else:
        op = op[:-1]

    return op
