-- All we need is to make sure that the name of the table is the same as the name of the file ( except for the .lua )
-- Right now it searches for trackname_preloop.mp3 and trackname_loop.mp3, though if you want to change it to _pre thats fine as well
-- It assumes that all files are in /Data/Sounds/Streams so you don't need the full path
-- So this is the tedious part of switching to soundstream, if you could create one of these for all the other banks we are using
-- then we should be able to switch
-- 20080116_piano
-- l2p1

MusicSwarm09 = 
{	[ 1 ] = "musicswarm09_piano"
,	[ 2 ] = "musicswarm09_basses"
,	[ 3 ] = "musicswarm09_cellos"
,	[ 4 ] = "musicswarm09_oboe"
,	[ 5 ] = "musicswarm09_horn"
,	[ 6 ] = "musicswarm09_clarinet"
,	[ 7 ] = "musicswarm09_violins"
,	[ 8 ] = "musicswarm09_bassoon"
}

MusicSwarm09Volume = 0.7