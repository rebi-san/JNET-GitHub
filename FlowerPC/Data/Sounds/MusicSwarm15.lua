-- All we need is to make sure that the name of the table is the same as the name of the file ( except for the .lua )
-- Right now it searches for trackname_preloop.mp3 and trackname_loop.mp3, though if you want to change it to _pre thats fine as well
-- It assumes that all files are in /Data/Sounds/Streams so you don't need the full path
-- So this is the tedious part of switching to soundstream, if you could create one of these for all the other banks we are using
-- then we should be able to switch
-- 20080408
-- Sets key to "10" for C major
-- l3p1

MusicSwarm15 = 
{	[ 1 ] = "musicswarm15_violas"
,	[ 2 ] = "musicswarm15_pianocellos"
,	[ 3 ] = "musicswarm15_horn"
,	[ 4 ] = "musicswarm15_oboeviolins1"
,	[ 5 ] = "musicswarm15_clarinettrumpet"
,	[ 6 ] = "musicswarm15_bassoons"
,	[ 7 ] = "musicswarm15_violins2"
,	[ 8 ] = "musicswarm15_basses"
}

MusicSwarm15Volume = 1.0