Audio      {#audio}
===================

The Sifteo Base provides audio from a single speaker, and global volume can be controlled by the slider on the Sifteo Base. 

![](@ref base-render-512.png)

## Audio Format

All audio is rendered at a rate of __16kHz__, and audio data is expected to be in __signed 16-bit__ samples.

As with the rest of the data in your .elf binary, audio data is fetched from external storage before it can be used. Taking this into consideration, audio can be stored in two formats:

### ADPCM (compressed)

By default `stir` will compress your audio data to ADPCM, an efficient format that compresses to 25% of the size of the source audio. This is the preferred format for most audio, unless you have a specific reason to leave it uncompressed.

Compressed audio data fetched from external storage remains compressed in the cache, and is decompressed for playback.

@note We use a variant of ADPCM which includes a "fast start" feature, to help the codec quickly adapt to the initial conditions at the beginning of a sample. If you've experienced problems with using other forms of ADPCM with looped or very short samples, rest assured that our codec has been designed with these cases in mind.

### PCM (uncompressed)

Audio assets may also be stored uncompressed, resulting in larger binaries, but avoiding the small amount of work required to decompress the audio.

Typically PCM audio is only useful if you will be generating the audio data programmatically, if you expect your data to have a highly nonlinear access pattern, or if your application cannot tolerate the small amount of distortion introduced by the lossy ADPCM compression.

## Channels

__8 channels__ of audio are available on the Sifteo Base. A single Sifteo::AudioChannel can render one Sifteo::AssetAudio at a time.

The Sifteo::AudioTracker, described below, can also play back multichannel musical compositions, while typically sound effects will be played back directly on a single channel.

@note The total of 8 channels must be shared between the tracker and any sound effects that need to play simultaneously. For instance, if your tracker module uses 5 channels, only 3 channels are available for sound effects.

## Sample Playback

The simplest way to generate audio is to play back a sample. Samples can be played back by specifying a `sound` in your `assets.lua` file, which will prompt `stir` to create a Sifteo::AssetAudio. This can be played back by a Sifteo::AudioChannel:

~~~~~~~~~~~~~
Sifteo::AudioChannel(0).play(SampleToEndAllSamples);
~~~~~~~~~~~~~

See @ref asset_workflow for more details on how to prepare your audio and integrate it into your build.

## Tracker

The Sifteo Base can also play tracker modules. Tracker modules are particularly interesting since they allow us to efficiently render longer pieces of audio, which would otherwise be prohibitively large to store on the Base.

From a very high level, we can think of a tracker module as being played back by a sequencer. The module (or 'mod'), provides a number of samples, usually for several different instruments, and contains events that specify when those samples should be played. There are also a variety of effects that can be applied to a sample being played back in the tracker.

A variety of slightly differing module formats have emerged over the years - the Sifteo tracker is compatible with the XM file format.

Once you've added your mod to `stir`, playing it back is as simple as:

~~~~~~~~~~~~~
Sifteo::AudioTracker::play(ModToRuleTheWorld);
~~~~~~~~~~~~~

### Input

`stir` will accept almost any XM module, so long as some limitations (below) are met. It's recommended that you use [MilkyTracker](http://www.milkytracker.org) or [MODPlug](http://www.modplug.com/trackerinfo.html) as your authoring tool, as the Sifteo tracker has been designed to be as compatible as possible with them.

### Output

Stir compresses modules in a few ways. All samples (accepted formats: pcm16, pcm8, and adpcm) are compressed to ADPCM (like a normal Sifteo::AssetAudio), and envelopes are also compressed. Samples are also deduplicated so multiple songs can share the same sample data, further saving space on the Base.

### Performance

Tracker modules can be a very efficient format, but they can also be terrifically inefficient if abused. Here are some tips to keeping your modules tight and fast.

* __Fewer Channels__: Minimize the number of channels required by your module, whenever possible. The fewer channels the Base has to render, the more time it has to devote to other important work.

* __High Sample Rates__: Samples in XM are traditionally 8363 Hz at C-4. This provides a wide range of timbre options while providing a compact sample. The instrument's Relative Note changes the sampling rate by a constant amount, so that an instrument with a higher Relative Note will sample it's audio much faster than an instrument with a lower Relative Note. Higher sampling rates mean higher-quality samples, but also more storage space required in flash, and more flash bus usage during playback. Samples with a Relative Note greater than 0 should be carefully scrutinized. Usually they indicate a 44.1kHz sample that was imported directly into the tracker without resampling.

* __Ping Pong Loops__: The ADPCM encoding does not provide random access, and is monotonic. So, to provide ping pong loops, `stir` converts the ping pong loop into a linear loop, doubling the loop's size. While ping pong loops can be incredibly useful, they can take up significantly more space in flash and should be used sparingly.

* __Empty Space__: Samples containing empty space are samples that are wasting space. Many samples lead in with some silence or lead out with a long fade. Consider using effect __ED__ (Note delay) instead of leading a sample with silence, and trimming the silence from the end of your sample to save storage space.

* __Relative Note Adjustment__: For the particularly frugal: if an instrument is only played in the higher registers, it's beneficial to use a lower Relative Note. The Base mixes at 16kHz, so an instrument with a Relative Note of 0 playing a C-5 will be mixing very near the mixer's native rate. Higher notes will be oversampled, wasting flash bandwidth.

### Effects

Practically all XM features are available. Unfortunately, this does not mean that all XM features will run __well__ on the Base. Due to the unique constraints of our hardware, the following features should be avoided when possible:

__9xx (Sample offset)__: Samples on the Base do not provide random access, so jumping to an offset in a sample requires fetching the rest of the sample before the offset as well. In cases where the offset is large and the effect is used on multiple channels at the same time, this can noticeably affect performance.

__Dxx (Pattern break)__: Patterns, like samples, do not provide random access and must be read from the beginning of the pattern to the requested offset. As such, executing a pattern break and jumping more than a few rows into the next pattern can be expensive.

__E6x (Pattern loop)__: It is possible to use pattern loops in an efficient manner, but due to the linear access nature of patterns they can also be very inefficient. Where possible, pattern loops should start as near to the beginning of the pattern as possible—looping to the very beginning of a pattern is as cheap as stepping to the next row in the same pattern.

The least efficient pattern loop would begin the loop on the second to last row of the pattern, with a duration of one row. This would cause the entire pattern to be read from flash on every loop division.

### Channels

The Base Cube provides 8 channels of audio, and all of them can be used for tracker playback, but before creating a song using all 8 channels, there are a few reasons why fewer is better:

* Channels being used to play your song are channels that are not available to be used for sound effects. A song with 8 channels leaves you with no free channels for dynamic sound effects when a user interacts with your game.
* Patterns are the second largest consumers of flash after samples (potentially the largest, if you're taking advantage of sample deduplication across modules), and their size increases linearly with the number of channels.

Lastly, editing tools do not appear to have a way to add or remove channels after a song has been created. While `stir` should handle this in the future, it does not today and your songs will be larger and have the potential to conflict with standard sound effects. To mitigate this, the tracker allocates channels from the top down in the mixer, so a song that has eight channels but only uses channels one through four (trackers like to one-index their numbering) will leave channels zero through three (we use zero-indexing) free on the Base.

### Limitations and Unavailable Effects

The following constraints are imposed on tracker modules:

* Panning is not available, since we only have a single speaker. Any panning information is discarded.
* No empty instruments
* No empty patterns
* One sample per instrument

__Automatic instrument vibrato__ is not yet available.

The following effects are not implemented, and likely inconsistent between MilkyTracker, FastTracker II, and MODPlug, and will not be available:

* Set gliss control (E3)
* Set tremolo control (E7)

All other effects and all volume column effects are available.

### References

The XM file specification can be found [here](ftp://ftp.heanet.ie/disk1/sourceforge/u/project/uf/ufmod/XM%20file%20format%20specification/FastTracker%20II%2C%20ADPCM%20XM%20and%20Stripped%20XM/XM_file_format.pdf.gz), but the [MilkyTracker documentation](http://www.milkytracker.org/docs/MilkyTracker.html) is significantly more useful for understanding effects. The MOD specification from which it inherits many of its features can be found [here](http://147.91.177.212/extra/fileformat/modules/mod/mod-form.txt).
