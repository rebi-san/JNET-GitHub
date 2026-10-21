-- All we need is to make sure that the name of the table is the same as the name of the file ( except for the .lua )
-- Right now it searches for trackname_preloop.mp3 and trackname_loop.mp3, though if you want to change it to _pre thats fine as well
-- It assumes that all files are in /Data/Sounds/Streams so you don't need the full path
-- So this is the tedious part of switching to soundstream, if you could create one of these for all the other banks we are using
-- then we should be able to switch
-- 20080913d
-- 20081201f

MusicSwarmL62 = 
{	[ 1 ] = "l6p2_tuba"
,	[ 2 ] = "l6p2_bassdrumharp"
,	[ 3 ] = "l6p2_cellos"
,	[ 4 ] = "l6p2_chimes"
,	[ 5 ] = "l6p2_violins"
,	[ 6 ] = "l6p2_soloflute"
,	[ 7 ] = "l6p2_soloviolin"
,	[ 8 ] = "l6p2_piano"
}

-- MusicSwarmL62 = 
-- {	[ 1 ] = "l6p2_cymbals"
-- ,	[ 2 ] = "l6p2_bassdrum"
-- ,	[ 3 ] = "l6p2_harp"
-- ,	[ 4 ] = "l6p2_piano"
-- ,	[ 5 ] = "l6p2_clarinet"
-- ,	[ 6 ] = "l6p2_violin"
-- ,	[ 7 ] = "l6p2_bassoon"
-- ,	[ 8 ] = "l6p2_flute"
-- }


-- MusicSwarmL62 = 
-- {	[ 1 ] = "20080913d_bassdrum"
-- ,	[ 2 ] = "20080913d_cymbalfx"
-- ,	[ 3 ] = "20080913d_harp"
-- ,	[ 4 ] = "20080913d_piano"
-- ,	[ 5 ] = "20080913d_clarinet"
-- ,	[ 6 ] = "20080913d_violin"
-- ,	[ 7 ] = "20080913d_bassoon"
-- ,	[ 8 ] = "20080913d_flute"
-- }



MusicSwarmL62Volume = 1.1