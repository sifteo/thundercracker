
Device Management       {#device_mgmt}
=================

# Overview              {#overview}

`swiss` is a utility tool, provided with the SDK to perform a variety of device management tasks. `swiss` can be found in __sdk/bin__ within the Sifteo SDK.

You can easily use `swiss` from within an SDK shell - just double click on the appropriate launcher in the SDK:
* Windows: double click `sifteo-sdk-shell.cmd`
* Mac OS X: double click `sifteo-sdk-shell.command`
* Linux: run `sifteo-sdk-shell.sh`

# App Installation      {#installation}

To install a new version of your application to your Sifteo base, use the `swiss install` command as follows:

    swiss install myapplication.elf

This will remove any previously installed versions of `myapplication.elf`, and install the new one. There is currently no restriction on installing a new version while your application is running, but it's not guaranteed to work, and not recommended!

@note You must call Sifteo::Metadata::package() within your game's source code in order to generate the information required to install it successfully.

More commands coming soon...
