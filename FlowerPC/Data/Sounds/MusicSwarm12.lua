-- All we need is to make sure that the name of the table is the same as the name of the file ( except for the .lua )
-- Right now it searches for trackname_preloop.mp3 and trackname_loop.mp3, though if you want to change it to _pre thats fine as well
-- It assumes that all files are in /Data/Sounds/Streams so you don't need the full path
-- So this is the tedious part of switching to soundstream, if you could create one of these for all the other banks we are using
-- then we should be able to switch
-- 20080206_harp
-- l2p3 - 1:13:10

MusicSwarm12 = 
{	[ 1 ] = "musicswarm12_harp"
,	[ 2 ] = "musicswarm12_piano"
,	[ 3 ] = "musicswarm12_basses"
,	[ 4 ] = "musicswarm12_horns"
,	[ 5 ] = "musicswarm12_clarinet"
,	[ 6 ] = "musicswarm12_violas"
,	[ 7 ] = "musicswarm12_oboe"
,	[ 8 ] = "musicswarm12_violins"
}

MusicSwarm12Volume = 0.7