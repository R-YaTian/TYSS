# TYSS for 3DS
TYSS or RYaTian's Save Studio for 3DS.

* Circle Pad Left/Right, C-Stick Left Left/Right or ZL/ZR Changes View Type. Introduces L/R button function toggle, which allows Old 3DS users to switch View Type using the L/R buttons.
* While in title selection:
    * A Opens the highlighted title
	* Y Adds the highlighted title to favorites.
	* X Open options menu

* While Backup Item Selection is open:
    * A Overwrites an already dumped backup or selects New
	* X Deletes the highlighted item
	* Y Restores the highlighted item
	* R Upload the highlighted zip/sav/bin file
	* B Closes and returns to title select

**Google Drive Instructions:**
Follow the guide [here](./GD_INSTRUCTIONS.MD).

**NOTE: Cloud Drive feature requires backup created with the `Export To Zip` option turned on are uploadable.**

**See [releases](https://github.com/R-YaTian/TYSS/releases) for the latest version.** 

## Building:
* Setup [devkitPro](https://www.3dbrew.org/wiki/Setting_up_Development_Environment)
* Run this command to install dependencies:
	* `dkp-pacman -S 3ds-curl 3ds-mbedtls 3ds-zlib` (You may need to remove `dkp`)
	* `pip install zopflipy` (You may need to install python and pip first)
	* Install `gettext` toolchain via your package manager
* Then simply run `make`
