
Device Management       {#device_mgmt}
=================

@brief How to install and manage content on the Sifteo Base

# Overview              {#overview}

`swiss` is a utility tool, provided with the SDK to perform a variety of device management tasks. `swiss` can be found in __sdk/bin__ within the Sifteo SDK.

You can easily use `swiss` from within an SDK shell - just double click on the appropriate launcher in the SDK:
* Windows: double click `sifteo-sdk-shell.cmd`
* Mac OS X: double click `sifteo-sdk-shell.command`
* Linux: run `sifteo-sdk-shell.sh`

# Install Apps          {#install}

To install a new version of your application to your Sifteo base, use the `swiss install` command as follows:

    swiss install myapplication.elf

This will remove any previously installed versions of `myapplication.elf`, and install the new one. It's even possible to install a new version while your application is running. The old version will continue to run until you restart the app.

To install an app that should be used as the launcher menu, use the `-l` flag:

    swiss install -l mylauncher.elf

@note You must call Sifteo::Metadata::package() within your game's source code in order to generate the information required to install it successfully.

# Manifest              {#manifest}

The `manifest` command provides a summary of the content installed on your Sifteo base. Invoke it with:

    swiss manifest

Depending on what you have installed, you'll see some results similar to those below:

    C:\code\sifteo-sdk\examples>swiss manifest
    System: 128 kB  Free: 15104 kB  Firmware: ---
    VOL TYPE     ELF-KB OBJ-KB PACKAGE                 VERSION TITLE
    0d  Launcher    128      0 com.sifteo.launcher     0.1     System Launcher
    10  Game        128      0 com.sifteo.extras.hello 0.1     Hello World SDK Example

# Delete Content        {#delete}

Swiss provides a few options for removing content from your Sifteo base. To remove all content from your Sifteo base, use the `--all` flag:

    swiss delete --all

To remove a single game, delete it by specifying its __volume__.

You can retrieve the volume number of a game from the `manifest` command above. For example, in the manifest output above the "Hello World SDK Example" is listed as __10__ in the __VOL__ column, so the command to remove it is:

    swiss delete 10

You can also remove all system bookkeeping information from the base, including Sifteo::AssetSlot allocation records.

    swiss delete --sys

Once you delete this system info, the base will need to install assets to your cubes the next time you play a game, even if those assets were previously installed.

# Update Firmware       {#fwupdate}

Swiss can also update the firmware on your Sifteo base.

This is a two step process. First, the base must be in update mode. If the red LED is already illuminated, the base is already in update mode and you can skip this step. Otherwise, to initialize the updater on the base:

    swiss update --init

Your base will disconnect from USB and reboot into update mode. When the red LED is illuminated, your device is in update mode.

Once in update mode, you can install new firmware:

    swiss update myNewFirmware.sft

## Recovery             {#recovery}

If you need to force an update, you can use the following recovery process:
* remove batteries from the base and disconnect from USB
* hold down the home button on the base
* connect the base via USB (this will turn on the base)
* continue holding down the home button on the base for one second

After one second, the red LED illuminates and you can install the update as normal:

    swiss update myNewFirmware.sft
