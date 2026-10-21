-- All we need is to make sure that the name of the table is the same as the name of the file ( except for the .lua )
-- Right now it searches for trackname_preloop.mp3 and trackname_loop.mp3, though if you want to change it to _pre thats fine as well
-- It assumes that all files are in /Data/Sounds/Streams so you don't need the full path
-- So this is the tedious part of switching to soundstream, if you could create one of these for all the other banks we are using
-- then we should be able to switch
-- 20080616 (old)
-- 20081006 (new)
-- l4p1

MusicSwarm19 = 
{	[ 1 ] = "20081006_bassoon"
,	[ 2 ] = "20081006_piano1"
,	[ 3 ] = "20081006_piano2"
,	[ 4 ] = "20081006_strings"
,	[ 5 ] = "20081006_guitar"
}

MusicSwarm19Volume = 0.7