-- All we need is to make sure that the name of the table is the same as the name of the file ( except for the .lua )
-- Right now it searches for trackname_preloop.mp3 and trackname_loop.mp3, though if you want to change it to _pre thats fine as well
-- It assumes that all files are in /Data/Sounds/Streams so you don't need the full path
-- So this is the tedious part of switching to soundstream, if you could create one of these for all the other banks we are using
-- then we should be able to switch
-- 20080326
-- Sets key to "9" for C# major
-- l3p2

MusicSwarm14 = 
{	[ 1 ] = "musicswarm14_piano"
,	[ 2 ] = "musicswarm14_cellos"
,	[ 3 ] = "musicswarm14_violins1"
,	[ 4 ] = "musicswarm14_hornsviolas"
,	[ 5 ] = "musicswarm14_oboes"
,	[ 6 ] = "musicswarm14_trombonesviolins2"
,	[ 7 ] = "musicswarm14_clarinetbassoons"
,	[ 8 ] = "musicswarm14_basses"
}

MusicSwarm13Volume = 1.0
