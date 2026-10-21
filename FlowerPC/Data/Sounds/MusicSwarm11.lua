-- All we need is to make sure that the name of the table is the same as the name of the file ( except for the .lua )
-- Right now it searches for trackname_preloop.mp3 and trackname_loop.mp3, though if you want to change it to _pre thats fine as well
-- It assumes that all files are in /Data/Sounds/Streams so you don't need the full path
-- So this is the tedious part of switching to soundstream, if you could create one of these for all the other banks we are using
-- then we should be able to switch
-- 20080130_guitar1
-- l2p2

MusicSwarm11 = 
{	[ 1 ] = "musicswarm11_guitar1"
,	[ 2 ] = "musicswarm11_piano"
,	[ 3 ] = "musicswarm11_guitar2"
,	[ 4 ] = "musicswarm11_cellos"
,	[ 5 ] = "musicswarm11_bassflute"
,	[ 6 ] = "musicswarm11_horn"
,	[ 7 ] = "musicswarm11_clarinet"
}

MusicSwarm11Volume = 0.8