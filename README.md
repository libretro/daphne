This is a Daphne core.  
  
  
Source Roots
------------
Original Source:  http://www.daphne-emu.com/download/daphne-1.0-src.tar.bz2  
Main Author:      Matt Ownby  
Other Authors:    Mark Broadhead, Warren Ondras, Paul Blagay, Scott Duensing, Andrew Hepburn, Robert DiNapoli  
  
Later Source:     https://github.com/mirror/daphne-emu/tree/master/doc  
- Not too much action on that repo.
- Quick glance shows most changes are in the Singe area which is not tested and, therefore, likely broken.
  
  
Quick Source History to Initial Commit
--------------------------------------
This core was based on the "Original Source" as above, which was released under GPL.
- Android was the first target system, and was the only supported system in the Initial Commit.
- Original source was ported from using Android incompatible SDL1 to SDL2.
- Then the source was prepared to be a standalone Android APK.  A successful APK version was created but not released, as a Libretro core was thought to be the best mechanism for release.
- Finally, the code base coming a long way, we come to this core that you're looking at.
- Other platform code was left in place and even ported in a few places.
- Singe code was never tested or compiled in.
  
  
Game Compatibility at Initial Commit
------------------------------------
- Generally mimics original Daphne compatibility - it was never compatible with every Laser Disc game.
- "Test" type and Pioneer gear based cores were ignored.
- Key for "S" column:  
|S|Meaning                                     |
|-|--------------------------------------------|
|-|See notes                                   |
|X|Not tested                                  |
|*|Tested and working (as well as it ever did) |
  
  
|Core          |Short Name      |S|Notes                                                                                            |
|--------------|----------------|-|-------------------------------------------------------------------------------------------------|
|astron        |astronp         |-|"p" denotes running on Pioneer gear, so unnecessary for Android                                  |
|              |blazer          |X|Never found a good Laser Disc                                                                    |
|              |galaxyp         |-|"p" denotes running on Pioneer gear, so unnecessary for Android                                  |
|              |astron          |*|                                                                                                 |
|              |galaxy          |*|                                                                                                 |
|              |cobraab         |*|                                                                                                 |
|badlands      |badlands        |*|                                                                                                 |
|              |badlandsp       |-|"p" denotes running on Pioneer gear, so unnecessary for Android                                  |
|bega          |bega            |*|                                                                                                 |
|              |begar1          |*|                                                                                                 |
|              |cobra           |*|known gfx issues, like cockpit being clipped, still can play                                     |
|              |roadblaster     |*|                                                                                                 |
|benchmark     |benchmark       |-|unneeded, so untested                                                                            |
|cliff         |cliff           |*|                                                                                                 |
|              |gtg             |-|Goal to Go, was supposedly broken because of fixes for cliff                                     |
|              |cliffalt        |*|                                                                                                 |
|              |cliffalt2       |*|                                                                                                 |
|cobraconv     |cobraconv       |-|Cobra Command conversion, this has never worked                                                  |
|cputest       |cputest         |-|unneeded, so untested                                                                            |
|esh           |esh             |*|                                                                                                 |
|              |eshalt          |*|                                                                                                 |
|              |eshalt2         |*|                                                                                                 |
|ffr           |ffr             |-|Freedom Fighter, this has never worked                                                           |
|firefox       |firefox         |-|Firefox, code was only ever partially done                                                       |
|              |firefoxa        |-|Firefox v2, code was only ever partially done                                                    |
|gpworld       |gpworld         |*|This is a multi-screen game, known to be clunky                                                  |
|interstellar  |interstellar    |*|                                                                                                 |
|lair          |lair            |*|                                                                                                 |
|              |lair_f          |*|                                                                                                 |
|              |lair_e          |*|                                                                                                 |
|              |lair_d          |*|                                                                                                 |
|              |lair_c          |*|                                                                                                 |
|              |lair_b          |*|                                                                                                 |
|              |lair_a          |*|                                                                                                 |
|              |dle11           |*|                                                                                                 |
|              |dle21           |*|Was showing some sound corruption at times on load                                               |
|              |dle20           |*|                                                                                                 |
|              |ace             |*|                                                                                                 |
|              |ace_a2          |*|                                                                                                 |
|              |ace_a           |*|                                                                                                 |
|              |sae             |*|Boot is normally long                                                                            |
|              |lair_n1         |*|                                                                                                 |
|              |lair_x          |*|Some ROM files need renaming from DLUn.bin to dl_x_un.bin                                        |
|lair2         |lair2           |*|See: https://www.daphne-emu.com:9443/phpBB3/viewtopic.php?t=2732 about lair2 versions not working|
|              |lair2_318       |*|                                                                                                 |
|              |lair2_317       |-|Code was all doc'd out in original source, implying there was never a 317                        |
|              |lair2_315       |*|                                                                                                 |
|              |lair2_314       |-|Error: EEP unhandled OPCode 0 with address 3, see link in lair2                                  |
|              |lair2_300       |-|Long boot, input not working                                                                     |
|              |lair2_211       |-|Can't get past the Monitor Test, same error as 314                                               |
|              |ace91           |-|Supported as well as Daphne can, in that it's not, no diagonal input                             |
|              |ace91_euro      |-|See ace91                                                                                        |
|              |lair2_316_euro  |*|Sometimes get error from 314 but doesn't crash                                                   |
|              |lair2_319_euro  |*|                                                                                                 |
|              |lair2_319_span  |*|Sometimes an odd corruption in top 10% of screen which quickly goes away                         |
|laireuro      |laireuro        |-|Untested, needs new framefile and mpeg with 720x576, should just work                            |
|              |aceeuro         |-|See laireuro                                                                                     |
|              |lair_ita        |-|See laireuro                                                                                     |
|              |lair_d2         |-|See laireuro                                                                                     |
|lpg           |lpg             |-|Laser Grand Prix, code was only ever partially done                                              |
|mach3         |mach3           |*|                                                                                                 |
|              |uvt             |*|                                                                                                 |
|              |cobram3         |*|                                                                                                 |
|multicputest  |mcputest        |-|unneeded, so untested                                                                            |
|seektest      |seektest        |-|unneeded, so untested                                                                            |
|singe         |singe           |-|Untested.                                                                                        |
|speedtest     |speedtest       |-|unneeded, so untested                                                                            |
|starrider     |starrider       |-|Star Rider, code was only ever partially done                                                    |
|superd        |sdq             |*|                                                                                                 |
|              |sdqshort        |*|                                                                                                 |
|              |sdqshortalt     |*|                                                                                                 |
|test_sb       |test_sb         |-|unneeded, so untested                                                                            |
|thayers       |tq              |*|Need keyboard and sound to effectively play                                                      |
|              |tq_alt          |*|                                                                                                 |
|              |tq_swear        |*|                                                                                                 |
|timetrav      |timetrav        |-|Time Traveler, code was only ever partially done                                                 |
  
  
Development Notes
-----------------
- Using Vulkan and Double Buffering can cut input latency by 16ms.
- Recommendations: Vulkan driver, Threaded Video: off, Vsync: on, Max Swapchain Images: 2, Input Poll Type: Early
  
  
Directory Setup
---------------
Initial path taken from RA during load, user points to a ROM zip file like lair.zip.  
  
### Example:
1. Comes from RA:	/storage/emulated/0/Roms/Daphne/roms/lair.zip
2. Then it's stripped down: 
   - Name: lair
   - Extension: zip
   - home_dir (where everything hangs off): /storage/emulated/0/Roms/Daphne/roms/..
3. Directories given above:  
|Type       |Directory                                                   |
|-----------|------------------------------------------------------------|
|pics:      |[home_dir]/pics                                             |
|ram:       |[home_dir]/ram                                              |
|sound:     |[home_dir]/sound                                            |
|framefile: |[home_dir]/framefile                                        |
|CDROM:     |[framefile_dir]/[first line in the framefile (IE lair.txt)] |
  
The only lair.txt that is considered is in framefile.
