-- All we need is to make sure that the name of the table is the same as the name of the file ( except for the .lua )
-- Right now it searches for trackname_preloop.mp3 and trackname_loop.mp3, though if you want to change it to _pre thats fine as well
-- It assumes that all files are in /Data/Sounds/Streams so you don't need the full path
-- So this is the tedious part of switching to soundstream, if you could create one of these for all the other banks we are using
-- then we should be able to switch
-- 20080603
-- l1p1

MusicSwarm18 = 
{	[ 1 ] = "musicswarm18_guitar"
,	[ 2 ] = "musicswarm18_strings"
,	[ 3 ] = "musicswarm18_piano"
}

MusicSwarm18Volume = 0.7