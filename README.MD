# JKSM for 3DS
JKSM or JK's Save Manager for 3DS.

* Circle Pad Left or Right Changes Save Data Type
* While in title selection:
    * A Opens the highlighted title
	* Y Adds the highlighted title to favorites.
	* More later

* While Folder Selection is open:
    * A Overwrites an already dumped backup or selects New
	* X Deletes the highlighted folder
	* Y Restores the highlighted folder
	* B Closes and returns to title select

**Google Drive Instructions:**
Follow the guide [here](./GD_INSTRUCTIONS.MD). For 3DS, step 24 is a bit different as JKSM 3DS does not have a text config file. Instead, create a file named `drive.json` and use this as the template:
```
{
    "driveClientID" : "YOUR_DRIVE_CLIENT_ID_HERE",
    "driveClientSecret" : "YOUR_DRIVE_CLIENT_SECRET",
    "driveAuthCode" : "YOU_CODE_FROM_STEP_22_HERE"
}
```
Place `drive.json` inside the `JKSV` folder in the root of your SD card. 

**NOTE: Google Drive support currently requires saves created with the `Export To Zip` option turned on are uploadable for now.**

**See [releases](https://github.com/R-YaTian/JKSM/releases) for the latest version.** 

## Building:
* Setup [devkitPro](https://www.3dbrew.org/wiki/Setting_up_Development_Environment)
* Run this command. You may need to remove `dkp`:
	* `dkp-pacman -S 3ds-curl 3ds-libjson-c 3ds-mbedtls 3ds-zlib`
