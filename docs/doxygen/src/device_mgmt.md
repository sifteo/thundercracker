
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

    $ swiss install myapplication.elf

This will remove any previously installed versions of `myapplication.elf`, and install the new one. It's even possible to install a new version while your application is running. The old version will continue to run until you restart the app.

To install an app that should be used as the launcher menu, use the `-l` flag:

    $ swiss install -l mylauncher.elf

@note You must call Sifteo::Metadata::package() within your game's source code in order to generate the information required to install it successfully.

# Manifest              {#manifest}

The `manifest` command provides a summary of the content installed on your Sifteo base. Invoke it with:

    $ swiss manifest

Depending on what you have installed, you'll see some results similar to those below:

    C:\code\sifteo-sdk\examples>swiss manifest
    System: 128 kB  Free: 15104 kB  Firmware: ---
    VOL TYPE     ELF-KB OBJ-KB PACKAGE                 VERSION TITLE
    0d  Launcher    128      0 com.sifteo.launcher     0.1     System Launcher
    10  Game        128      0 com.sifteo.extras.hello 0.1     Hello World SDK Example

# Retrieve Saved Data   {#savedata}

At runtime, your app may store persistent data via Sifteo::StoredObject - this could be metrics, game save data, or anything you like. swiss can retrieve this data for your inspection.

Sifteo::StoredObject provides a namespace of 255 keys for a given app. The data for each StoredObject is stored by key in a journaled filesystem - while the filesystem itself would only return to you the current value for a given key, there may be older versions of a key's data still lurking on disk before being garbage collected.

swiss can create a dump of all your app's StoredObject data, although there's no guarantee there will be more than one version available for each key. If more than one version is available, they're sorted from oldest to newest in the savedata file.

To retrieve savedata, use the `swiss savedata extract` command.

## Example

For instance, let's extract the save data for the __Hello World SDK Example__ shown in the manifest output above. Note that the __VOL__ code for this app is __10__, which we need to pass to swiss. The steps are as follows:

    $ swiss savedata extract 10 saved.bin

Now `saved.bin` contains all the StoredObject records that were found on the Base, in a simple binary format. Since StoredObject data is opaque to the system, we don't offer any standard way of formatting or parsing it. However, __tools/savedata.py__ within your SDK installation can serve as a template for parsing these files. By default, it will print savedata contents to stdout, dumping the hex value of user data. Invoke it as follows:

    $ python /path/to/sifteo-sdk/tools/savedata.py saved.bin

Example output might look like:

    /Users/liam/sifteo-sdk$ python tools/savedata.py out.bin
    header - key 0, val: com.sifteo.sdk.savedata
    header - key 1, val: 1.1
    header - key 2, val: 97983c9a-9527-4eeb-8d57-85a015ec70b4
    header - key 3, val: 33FFDB053347333251441843
    header - key 4, val: v0.9.10-48-gd44424b
    key 0, val (16 bytes): 00000000000000000000000000000000
    key 1, val (16 bytes): 01010101010101010101010101010101
    key 2, val (16 bytes): 02020202020202020202020202020202
    key 3, val (16 bytes): 03030303030303030303030303030303

@note Internally, Sifteo::StoredObject stores all elements with a size rounded up to the nearest multiple of 16. Ignore any extra padding as appropriate for your data.

# Delete Content        {#delete}

Swiss provides a few options for removing content from your Sifteo base. To remove all content from your Sifteo base, use the `--all` flag:

    $ swiss delete --all

To remove a single game, delete it by specifying its __volume__.

You can retrieve the volume number of a game from the `manifest` command above. For example, in the manifest output above the "Hello World SDK Example" is listed as __10__ in the __VOL__ column, so the command to remove it is:

    $ swiss delete 10

You can also remove all system bookkeeping information from the base, including Sifteo::AssetSlot allocation records.

    $ swiss delete --sys

Once you delete this system info, the base will need to install assets to your cubes the next time you play a game, even if those assets were previously installed.

# Update Firmware       {#fwupdate}

Swiss can also update the firmware on your Sifteo base.

This is a two step process. First, the base must be in update mode. If the red LED is already illuminated, the base is already in update mode and you can skip this step. Otherwise, to initialize the updater on the base:

    $ swiss update --init

Your base will disconnect from USB and reboot into update mode. When the red LED is illuminated, your device is in update mode.

Once in update mode, you can install new firmware:

    $ swiss update myNewFirmware.sft

## Recovery             {#recovery}

If you need to force an update, you can use the following recovery process:
* remove batteries from the base and disconnect from USB
* hold down the home button on the base
* connect the base via USB (this will turn on the base)
* continue holding down the home button on the base for one second

After one second, the red LED illuminates and you can install the update as normal:

    $ swiss update myNewFirmware.sft
