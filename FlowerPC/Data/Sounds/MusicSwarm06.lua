-- All we need is to make sure that the name of the table is the same as the name of the file ( except for the .lua )
-- Right now it searches for trackname_preloop.mp3 and trackname_loop.mp3, though if you want to change it to _pre thats fine as well
-- It assumes that all files are in /Data/Sounds/Streams so you don't need the full path
-- So this is the tedious part of switching to soundstream, if you could create one of these for all the other banks we are using
-- then we should be able to switch
-- 20071029_basses
-- l1p2

MusicSwarm06 = 
{	[ 1 ] = "musicswarm06_basses"
,	[ 2 ] = "musicswarm06_piano"
,	[ 3 ] = "musicswarm06_bassoons"
,	[ 4 ] = "musicswarm06_guitar"
,	[ 5 ] = "musicswarm06_clarinet"
,	[ 6 ] = "musicswarm06_cellos"
,	[ 7 ] = "musicswarm06_violinsviolas"
,	[ 8 ] = "musicswarm06_horn"
}

MusicSwarm06Volume = 0.8