itpl2dirtree

Create directory tree and symbolic link to entity according to
Mac iTnues playlist XML file (~/Music/iTunes/iTunes Music Library.xml).

itpl2dirtree is useful when you are coping iTunes library to non-iTunes
environment, such as Rasberry pi Volumio.

Depend package:
expat-dev

Please install expat-dev
for example
apt-get install expat-dev

Usage:
itpl2dirtree -p music data ath -i original music data path [-o output] [-n] < file.xml

-p: directory path name that holds music data
-i: prefix string, to be removed from data file path
-o: top directory path name, directory created in the path (default './playlist')
-n: dry run, read XML file and check it, but no output

stdin: iTunes library XML file, if file does not exists, try following steps
(https://support.apple.com/en-us/HT201610)

1. Open iTunes.
2. From the menu bar at the top of your computer screen, choose iTunes > Preferences.
3. Click the Advanced tab.
4. Select "Share iTunes Library XML with other applications.


To find prefix string, search "Location" for XML file, for example,
grep Location ~/Music/iTunes/'iTunes Music Library.xml' | head -1

if output is some of follows, use file:///Users/evalquote/Music/iTunes/iTunes%20Media/

<key>Location</key><string>file:///Users/evalquote/Music/iTunes/iTunes%20Media/Music/Kenneth%20Gilbert/Bach_%20Italian%20Concerto,%204%20Duos,%20Etc_/09%20Bach_%20Overture%20In%20The%20French%20Style,%20BWV%20831%20-%202.%20Courante.m4a</string>

Example:

On Mac
rsync -av --delete  ~/Music/iTunes/'iTunes Media/Music' volumio:Music/data
scp -p ~/Music/iTunes/'iTunes Music Library.xml' volumio:Music/itpl.xml

Then, on volumio
cd /mnt/USB
cat ~/Music/itpl.xml | itpl2dirtree -p ~/Music/data -i 'file:///Users/evalquote/Music/iTunes/iTunes%20Media/'

Enjoy!
