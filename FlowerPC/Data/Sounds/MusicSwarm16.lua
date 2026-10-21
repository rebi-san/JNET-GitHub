-- All we need is to make sure that the name of the table is the same as the name of the file ( except for the .lua )
-- Right now it searches for trackname_preloop.mp3 and trackname_loop.mp3, though if you want to change it to _pre thats fine as well
-- It assumes that all files are in /Data/Sounds/Streams so you don't need the full path
-- So this is the tedious part of switching to soundstream, if you could create one of these for all the other banks we are using
-- then we should be able to switch
-- 20080402
-- Sets key to "10" for C major
-- l3p3

MusicSwarm16 = 
{	[ 1 ] = "musicswarm16_piano"
,	[ 2 ] = "musicswarm16_violas"
,	[ 3 ] = "musicswarm16_clarinet"
,	[ 4 ] = "musicswarm16_bassoon"
,	[ 5 ] = "musicswarm16_basses"
}

MusicSwarm16Volume = 0.7