Journey: Rocket Death Match Mod, version 0.1.0
For Steam only

This mod aims to recreate how the unreleased "Rocket Death Match" DLC would have worked, if it hadn't been just a joke.


In Rocket Death Match, you move around in the style of a third-person shooter, and can shoot rockets that explode when they hit solid objects in the environment. If you or an online companion - rather, a competitor - get caught in a rocket blast, you will be launched into the air.

You are also able to meet competitors in Chapter Select, or return there together from other levels without being disconnected. From there, you can use the portals or the main exit to go to other levels together, just like normal. 
You can also meet competitors in the final level.

Please note that anywhere you are, you can only connect to other players who are also using this mod, in order to preserve the normal Journey experience for non-modding players.

CONTENTS
1. Installation
2. Starting Game
3. Controls
4. Menu Options
5. Other Features/Etc
6. Major Bugs (!!! Read to help avoid crashes !!!)
7. Minor "Bugs"
8. Credits

1. INSTALLATION

Copy everything in this archive into your Journey install folder, although the .CT cheat table file can go anywhere you want.

Change the file extension for Journey(RocketDeathMatch).txt from ".txt" to ".exe" to make the modded version of the game runnable.

You can rename Journey(RocketDeathMatch).exe to whatever you want and it should still work, although it might break if there are spaces in the name. If you rename it to Journey.exe you can open it from the Steam client, but you should back up the original Journey.exe first.

You will also need to have Cheat Engine installed, see https://cheatengine.org/ . The mod was made with Cheat Engine version 7.2, so you might have issues if you use an earlier version.
Please be aware that most antivirus/security software incorrectly detects Cheat Engine as malware, but it is safe in itself, and this mod only alters the behavior of the modded Journey game.


2. STARTING GAME

Open Journey(RocketDeathMatch).exe .
"Rocket Death Match" mode automatically activates at the start of every level when the opening cutscene ends.
However, you cannot see or interact with rockets fired by your competitors unless you activate the Cheat Engine script.
You should load the script at the game's title menu, right after the opening logos; it can also work if you load it at other times, but doing it right at the start of the game is safest and most effective.

Open Cheat Engine, then load "Rocket Death Match 0.1.0.CT" by clicking the button with a picture of a folder or using File > Load, navigating to the .CT file and clicking Open. This step can be done before or after you start Journey.

Then attach Cheat Engine to Journey by clicking the button with a picture of a computer monitor, or using File > Open Process, then select "Journey(RocketDeathMatch).exe" from the process list and click Open. 

Lastly, in the panel at the bottom of the Cheat Engine window, click the checkbox next to "Rocket Death Match - Multiplayer" to enable the script and make rockets work correctly in multiplayer.
Journey will slow down a lot for a short time while the script looks for the right data to modify, but this only happens when you load the script, and then the game should run normally the rest of the time (although it might be slightly slower than usual, but hopefully not much).


3. CONTROLS

Move your character and aim with the usual movement/camera controls.

Press the "Chirp" key/button to fire a rocket.
It should hit a spot very close to the center of the crosshair, though it may be slightly off depending on how far it travels; also if you turn the camera quickly and fire, then the rocket will probably shoot at where you were aiming a split second ago.

Tap the "Fly" key/button a few times to jump, and hold it down on the last tap to jump higher. Different tapping speeds/number of taps affects jump height.
Holding down "Fly" also makes you fall slower, get launched higher by exploding rockets, and slowly rise in water/float on top of it (but it's usually faster to go upward in water by just rocket-jumping up the side of a wall/etc).

Press the "Chirp", "Fly", and "Pause" keys/buttons at the same time to open the mod menu.
Navigate up/down in the menu with the movement controls, and select an option by pressing the "Chirp" key/button.


4. MENU OPTIONS

Resume: Closes mod menu.

Sit: Makes you sit down. Useful to prevent sliding while using the menu in Sunken City.

Chirp: Lets you simply do a normal chirp while you're pressing it, to help your competitor find you if needed, etc.

Fix Player Animations: Forces your competitor to stand up in your game - use this if they sit/fall down/etc and get stuck like that even while moving around, which is a common bug. This option (and Resume) also makes you yourself stand up.

Proceed in Chapter Select: Makes you return to Chapter Select, but you will still be proceeding in the same "Journey" - competitor's usernames are not erased from the list you see after the credits, etc - and you will stay connected to your current competitor as long as they also use "Proceed in Chapter Select" within about a minute. When you use this option, a message appears in your competitor's game for about half a minute, to let them know they can easily follow you.

There is a special event that happens if you are playing at midnight on December 31 or open the game anytime on January 1 - though midnight is most fitting - which unlocks a bonus menu option, as well as doing other things. You can also unlock the bonus option manually in the menu, if you are stubborn enough.


5. OTHER FEATURES / ETC.

This modded version of Journey uses the same SAVE.BIN file as the normal one, so your saves will overwrite each other, but any data saved with the mod should still work with the normal game and vice versa. When the mod saves, it saves you as a tier 4 white robe with the maximum possible scarf you could have at the start of the current level.

You will always be able to see where your competitor is moving in cutscenes/etc, since the multiplayer rocket script relies on that to work properly; also the Rocket Death Match mode allows you to move around at all times, and so it's more fun anyway to see both of you moving around and shooting rockets during cutscenes, instead of just seeing yourself.

The auto-disconnect distance between players has been doubled, since you are more likely to move far apart while battling. 
This might not always affect whether "lobby flag" triggers get activated for both players from that distance, so you might still get disconnected if you are very far apart and trigger a major event.

As stated above, you are able to meet competitors in Chapter Select and in the final level.


6. MAJOR BUGS

Sometimes, you might crash at the very end of the final level, right before the credits start - this might only happen if you and/or your competitior are shooting lots of rockets and jumping all over the place, so hopefully you will be fine if you just avoid doing that in the moments before the level ends.

The game sometimes crashes when you get attacked by a War Machine - this might only happen if you are shooting a rocket at the moment you are attacked, I'm not sure yet.

When you get close to the end of the final level, your game might freeze for a few seconds, but so far this does NOT seem to actually result in a crash, you just have to wait a little bit and then the game resumes.

Most levels will take a little longer than normal to load, and sometimes they might take quite a bit longer, but that will only appear to be a crash/freeze and it should still load within a few seconds.


7. MINOR "BUGS"

Rocket Death Match's built-in third-person-shooter movement system almost completely overrides normal movement - so you can move during cutscenes, but you also won't automatically slide down hills, and when in water/etc you will sink and usually can't jump with "Fly".
As stated above, you can still slowly rise by holding "Fly", plus you also still get launched upwards normally by rockets while underwater, so you can just stand next to a wall/etc and shoot it to rapidly rocket-jump your way upwards.

The explosion sounds always happen a short distance behind you - right where your camera usually is - instead of wherever a rocket actually explodes. The built-in code does this for some reason, and I don't know how to fix that yet.

You will always be wearing a tier 4 white robe, since the multiplayer rocket fix uses your robe "number" to send data between you and your competitor, and the "different robes" it uses are all displayed in-game as a tier 4 white robe.

You and your competitor's scarfs will grow/shrink whenever you fire a rocket, since the scarf length number is also used to send data. Fortunately, scarf length doesn't affect jump height or anything else in Rocket Death Match.

If you or your competitor sit down/fall down, you will still be able to move, but are usually stuck in a sitting/etc position for a long time. As stated above, this can be fixed by using the mod menu to force both players to stand up.

If you do a normal chirp (from the menu, etc) the "bubble" will usually be a lot bigger than normal, and cloth flyers attracted by it will wait longer than normal before they fly to you. These are side effects somehow caused by changing the level "lobbies" to prevent connecting with non-modding players. There might also be other weird issues like this, but nothing serious is known of so far.

8. CREDITS

Journey made by Thatgamecompany, including most of the Rocket Death Match code (thanks for leaving it in the game, hehe) 

Modded code/hacks/fixes to make the rockets work in multiplayer/etc, were made by Alazar88

Many memory address pointers for player avatars/etc used in mod were originally found by Rebi and Paleologos

Original version of "special event" audio was performed by Heline Fay