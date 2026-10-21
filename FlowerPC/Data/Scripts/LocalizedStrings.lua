-------------------------------------------------------------------------------
-- Notes:  A 2D table which is indexed on string ID and language.  Also
--         provides an accessor to the table.
--
-- Author: John Edwards
-- Date C: 11-10-2008
-------------------------------------------------------------------------------

-- This should probably be moved to a separate file ...
function GetLocalizedString( id )
	local strTbl = LocalizedStrings[ id ]
	if not strTbl then
		error( "No string with id: "..id )
	end

	-- Japan only wants japanese, US and Europe don't want japanese
	local languageName = SystemLanguageName()
	if REGION:find( "JP" ) then languageName = "japanese"
	elseif languageName:find( "japanese" ) then languageName = "english" 
	end

	local str = strTbl[ languageName ]
	if not str then
		DebugPrintError( "Unknown system language: "..SystemLanguageName()..", using English" )
		str = strTbl[ "english" ]
	end

	return str
end


-- Localized strings table.
LocalizedStrings =
{ BrokenSaveData =
  { japanese   = "本タイトルのロード中にエラーが発生し、\nセーブデータが上書きされます。\nロードをリトライしますか?"
  , english    = "There was an error loading the Flower™ save data\nand this data will be overwritten if you proceed.\nWould you like to retry loading the save data?"
  , french     = "Une erreur est survenue lors du chargement des données sauvegardées de Flower™.\nSi vous continuez, ces données seront écrasées.\nVoulez-vous à nouveau tenter de charger les données?"
  , spanish    = "Se ha producido un error al cargar los datos guardados de Flower™\ny se sobrescribirán si continúas.\n¿Quieres volver a intentar cargar los datos guardados?"
  , german     = [[Beim Laden der gespeicherten Flower™-Daten ist ein Fehler aufgetreten.\nWenn Sie fortfahren, werden die Daten überschrieben.\nMöchten Sie die gespeicherten Daten erneut laden?]]
  , italian    = "Si è verificato un errore durante il caricamento dei dati salvati di Flower™.\nContinuando, i dati saranno sovrascritti.\nVuoi riprovare a caricare i dati salvati?"
  , dutch      = "Het laden van de opgeslagen gegevens van Flower™ is mislukt.\nAls je verdergaat, worden deze gegevens overschreven.\nWil je de opgeslagen gegevens opnieuw laden?"
  , portuguese = "Ocorreu um erro a carregar os dados gravados Flower™\ne estes dados vão ser substituídos, se continuares.\nQueres voltar a tentar carregar os dados gravados?"
  , russian    = "Во время загрузки сохраненных данных Flower™\nпроизошла ошибка. Если вы продолжите, эти данные будут перезаписаны. \nПовторить попытку загрузить сохраненные данные?"
  }
, PS4BrokenSaveData =
  { japanese   = "セーブデータが破損していたためロードに失敗しました \n再度ロード致しますか？"
  , english    = "Load failed due to corrupted save data.\nAttempt load of Save Data again?"
  , french     = "Échec du chargement dû à des données sauvegardées endommagées.\nEssayer de charger à nouveau les données sauvegardées ?"
  , spanish    = "Se ha producido un error al cargar los datos guardados porque están dañados.\n¿Quieres intentar cargarlos de nuevo?"
  , german     = "Ladevorgang aufgrund beschädigter Daten fehlgeschlagen.\nErneut versuchen?"
  , italian    = "Caricamento fallito a causa di dati salvati corrotti.\nProvare a caricare nuovamente i dati salvati?"
  , dutch      = "Laden mislukt doordat de opslaggegevens zijn beschadigd.\nOpnieuw proberen opslaggegevens te laden?"
  , portuguese = "Erro ao carregar provocado por dados guardados corrompidos.\nTentar carregar os dados guardados outra vez?"
  , russian    = "Не удалось загрузить – сохраненные данные повреждены.\nПовторить попытку загрузки?"
  }
, PS4OverwriteBrokenSaveData =
  { japanese   = "破損したセーブデータに上書きします \n続けますか？"
  , english    = "Corrupt save data will be overwritten.\nProceed?"
  , french     = "Les données sauvegardées endommagées vont être écrasées.\nVoulez-vous continuer ?"
  , spanish    = "Los datos guardados dañados se sobrescribirán.\n¿Continuar?"
  , german     = "Beschädigte Daten werden überschrieben.\nFortfahren?"
  , italian    = "I dati salvati corrotti saranno sovrascritti.\Procedere?"
  , dutch      = "Beschadigde opslaggegevens worden overschreven.\nDoorgaan?"
  , portuguese = "Os dados guardados corrompidos serão substituídos\nProsseguir?"
  , russian    = "Поврежденные данные будут перезаписаны.\nПродолжить?"
  }
, GameName =
  { japanese   = "Flowery"
  , english    = "Flower"
  , french     = "Flower"
  , spanish    = "Flower"
  , german     = "Flower"
  , italian    = "Flower"
  , dutch      = "Flower"
  , portuguese = "Flower"
  , russian    = "Flower"
  }
, NeedSpace =
  { japanese   = "HDD（ハードディスク）の空き容量が足りないため、セーブできません。\n%dMB以上の空き容量が必要です。\nPSボタンを押して、「ゲーム終了」を選択して下さい。"
  , english    = "Not enough HDD space to continue, please have %dMB of free space. Press the PS button to quit the game.\n"
  , french     = "Espace insuffisant sur le disque dur, vous devez avoir %d MB d'espace libre pour continuer. Appuyez sur la touche PS pour quitter le jeu."
  , spanish    = "No hay suficiente espacio libre en el disco duro. Se necesitan, al menos, %dMB de espacio libre. Pulsa el botón PS para salir del juego."
  , german     = "Nicht genug freier Speicherplatz auf der Festplatte vorhanden. Bitte schaffe %d MB freien Speicherplatz. Drücke die PS-Taste, um das Spiel zu beenden."
  , italian    = "Spazio insufficiente sul disco fisso per continuare.\nOccorrono %d MB di spazio libero. Premi il tasto PS per uscire dal gioco."
  , dutch      = "Er is niet genoeg ruimte op de harde schijf om door te gaan. Zorg dat er %d MB beschikbaar is. Druk op de PS-toets om het spel af te sluiten."
  , portuguese = "Não existe espaço suficiente no disco rígido para continuar: tens de ter %dMB de espaço livre. Prime o botão PS para sair do jogo."
  , russian    = "Недостаточно свободного пространства на жестком диске. Для сохранения данных требуется %d Мб. Нажмите кнопку PS, чтобы покинуть игру."
  }
, NonOwnedSaveData =
  { japanese   = "セーブデータの所有者ではありません。\nトロフィーやゲームの進行をセーブできません。"
  , english    = "You are not the owner of the current save data.\nYou cannot earn Trophies or save progress."
  , french     = "Vous n'êtes pas propriétaire des données sauvegardées.\nVous ne pouvez pas gagner de trophée ou sauvegarder votre progression."
  , spanish    = "No eres el propietario de los datos guardados actuales.\nNo podrás ganar Trofeos ni guardar el progreso."
  , german     = "Du bist nicht Inhaber der momentan gespeicherten Daten.\nDu kannst keine Trophäen erhalten oder Fortschritte speichern."
  , italian    = "Non sei il proprietario dei dati salvati attuali.\nNon puoi ottenere i Trofei o salvare i progressi."
  , dutch      = "De opgeslagen gegevens zijn niet van jou.\nJe kunt geen prijzen verdienen en je vorderingen niet opslaan."
  , portuguese = "Não és o proprietário dos dados guardados actualmente.\nNão podes ganhar Troféus ou guardar a progressão."
  , russian    = "Вы не являетесь владельцем сохраненных данных.\nВы не можете зарабатывать награды или сохранять данные."
  }
, SaveDataDesc =
  { japanese   = "セーブデータ"
  , english    = "Save Data"
  , french     = "Sauvegarde des données"
  , spanish    = "Datos guardados"
  , german     = "Speicherdaten"
  , italian    = "Dati salvati"
  , dutch      = "Opgeslagen gegevens"
  , portuguese = "Dados guardados"
  , russian    = "Сохраненные данные"
  }
, FailedToSignInPSN =
  { japanese   = "PlayStation®Networkに接続できませんでした"
  , english    = "You failed to sign into PlayStation®Network. All online features will be disabled until you sign-in."
  , french     = "Échec de connexion à PlayStation®Network. Toutes les fonctions en ligne seront désactivées jusqu'à ce que vous vous connectiez."
  , spanish    = "No has iniciado sesión en PlayStation®Network. Todas las funciones online se deshabilitarán hasta que inicies sesión."
  , german     = "Du bist nicht bei PlayStation®Network angemeldet. Die Online-Funktionen werden erst aktiviert, wenn du dich angemeldet hast."
  , italian    = "Impossibile accedere a PlayStation®Network. Tutte le funzionalità online saranno disattivate finché non avrai effettuato l'accesso."
  , dutch      = "Het aanmelden bij PlayStation®Network is mislukt. De online functies worden uitgeschakeld totdat je je hebt aangemeld."
  , portuguese = "Não conseguiste iniciar sessão na PlayStation®Network. Todas as funcionalidades online serão desativadas até iniciares sessão."
  , russian    = "Вам не удалось войти в сеть PlayStation®Network. Сетевые возможности будут отключены, пока вы не войдете в сеть."
  }
}



-- per level feed text
CondensedText =
{
	en = "$USER_NAME_OR_ID dreamed $STORY_COUNT dreams.",
	nl = "$USER_NAME_OR_ID droomde $STORY_COUNT dromen.",
	fr = "$USER_NAME_OR_ID a fait $STORY_COUNT rêves.",
	de = "$USER_NAME_OR_ID hat $STORY_COUNT Träume geträumt.",
	it = "$USER_NAME_OR_ID ha fatto $STORY_COUNT sogni.",
	jp = "$USER_NAME_OR_ID は $STORY_COUNT の夢を見ました。",
	pt = "$USER_NAME_OR_ID sonhou $STORY_COUNT sonhos.",
	ru = "$USER_NAME_OR_ID видит сны. Их уже $STORY_COUNT.",
	es = "$USER_NAME_OR_ID tuvo $STORY_COUNT sueños."
}

FeedText =
{
	-- level 1
	{
		{
			en = "In a grassy field, $USER_NAME_OR_ID is savoring the warmth of the sun.",
			nl = "In een grasveld ligt $USER_NAME_OR_ID te genieten van de warme zon.",
			fr = "Dans une plaine verdoyante, $USER_NAME_OR_ID savoure la chaleur du soleil.",
			de = "$USER_NAME_OR_ID genießt auf einer Wiese die Wärme der Sonne.",
			it = "In un campo erboso, $USER_NAME_OR_ID assapora il tepore del sole.",
			jp = "豊かな緑のなか $USER_NAME_OR_ID は太陽のぬくもりを考えています。",
			pt = "Num campo relvado, $USER_NAME_OR_ID saboreia o calor do sol.",
			ru = "Среди широких полей $USER_NAME_OR_ID нежится на солнце.",
			es = "En un campo cubierto de hierba, $USER_NAME_OR_ID está disfrutando del calor del sol."
		}, {
			en = "Amidst the flowers, $USER_NAME_OR_ID is at peace.",
			nl = "Te midden van de bloemen komt $USER_NAME_OR_ID tot rust.",
			fr = "Parmi les fleurs, $USER_NAME_OR_ID se sent en paix.",
			de = "Inmitten der Blumen genießt $USER_NAME_OR_ID die Ruhe.",
			it = "In mezzo ai fiori, $USER_NAME_OR_ID è in pace.",
			jp = "$USER_NAME_OR_ID は花に囲まれ、休息を取っています。",
			pt = "No meio das flores, $USER_NAME_OR_ID encontra a paz.",
			ru = "Среди цветов $USER_NAME_OR_ID пребывает в спокойствии.",
			es = "En medio de las flores, $USER_NAME_OR_ID está en paz."
		}, {
			en = "On a breath of wind, $USER_NAME_OR_ID is surfing the skies.",
			nl = "Op een windvlaag surft $USER_NAME_OR_ID door de lucht.",
			fr = "Porté par une douce brise, $USER_NAME_OR_ID surfe dans les airs.",
			de = "$USER_NAME_OR_ID segelt auf einem Windhauch durch die Lüfte.",
			it = "Su un alito di vento, $USER_NAME_OR_ID naviga nei cieli.",
			jp = "$USER_NAME_OR_ID は風の中舞っています。",
			pt = "Num sopro de vento, $USER_NAME_OR_ID surfa os céus.",
			ru = "На крыльях ветра $USER_NAME_OR_ID летит в небеса.",
			es = "En un aliento del viento, $USER_NAME_OR_ID está navegando por los cielos."
		}
	},
	-- level 2
	{
		{
			en = "Surrounded by beauty, $USER_NAME_OR_ID is lost in the moment.",
			nl = "Omringd door schoonheid gaat $USER_NAME_OR_ID op in het moment.",
			fr = "Entouré par la beauté, $USER_NAME_OR_ID se perd dans la contemplation.",
			de = "Umgeben von Schönheit träumt $USER_NAME_OR_ID vor sich hin.",
			it = "Circondato dalla bellezza, $USER_NAME_OR_ID si dimentica di tutto.",
			jp = "$USER_NAME_OR_ID は美しさのあまり目がくらんでいます。",
			pt = "Apenas com beleza à sua volta, $USER_NAME_OR_ID perde-se no momento.",
			ru = "Среди красот $USER_NAME_OR_ID наслаждается моментом.",
			es = "En medio de la belleza, $USER_NAME_OR_ID se pierde en el momento."
		}, {
			en = "Among brown hills, $USER_NAME_OR_ID is finding new life.",
			nl = "Te midden van bruine heuvels vindt $USER_NAME_OR_ID nieuw leven.",
			fr = "Parmi les collines d'automne, $USER_NAME_OR_ID découvre de nouvelles vies.",
			de = "Auf braunen Hügeln findet $USER_NAME_OR_ID neues Leben.",
			it = "Tra colline brunastre, $USER_NAME_OR_ID trova una nuova vita.",
			jp = "茶色い丘の中、 $USER_NAME_OR_ID は新たな生命を見つけました。",
			pt = "Entre colinas castanhas, $USER_NAME_OR_ID encontra vida nova.",
			ru = "Среди бурых холмов $USER_NAME_OR_ID ищет новую жизнь.",
			es = "Entre las colinas marrones, $USER_NAME_OR_ID encuentra nueva vida."
		}, {
			en = "In a dream of color, $USER_NAME_OR_ID is listening to the stones.",
			nl = "In een droom van kleuren luistert $USER_NAME_OR_ID naar de stenen.",
			fr = "Dans un rêve en couleurs, $USER_NAME_OR_ID écoute les pierres.",
			de = "In einem bunten Traum lauscht $USER_NAME_OR_ID den Steinen.",
			it = "In un sogno colorato, $USER_NAME_OR_ID ascolta le pietre.",
			jp = "夢の中で $USER_NAME_OR_ID は石の音に耳を傾けています。",
			pt = "Num sonho cheio de cor. $USER_NAME_OR_ID escuta as pedras.",
			ru = "В цветном сне $USER_NAME_OR_ID прислушивается к камням.",
			es = "En un sueño de color, $USER_NAME_OR_ID está escuchando a las piedras."
		}
	},
	-- level 3
	{
		{
			en = "Weaving between windmills, $USER_NAME_OR_ID is dancing on the wind.",
			nl = "Tussen de windmolens door danst $USER_NAME_OR_ID op de wind.",
			fr = "Zigzaguant entre des moulins, $USER_NAME_OR_ID danse dans le vent.",
			de = "Zwischen Windmühlen verwoben tanzt $USER_NAME_OR_ID auf dem Wind.",
			it = "Zigzagando tra mulini a vento, $USER_NAME_OR_ID danza nel vento.",
			jp = "風車の間で $USER_NAME_OR_ID は風に舞っています。",
			pt = "Pelo meio dos moinhos, $USER_NAME_OR_ID dança ao sabor do vento.",
			ru = "Лавируя между ветряными мельницами, $USER_NAME_OR_ID танцует на ветру.",
			es = "Tejido entro los molinos de viento, $USER_NAME_OR_ID está bailando al son del viento."
		}, {
			en = "In a maze of canyons, $USER_NAME_OR_ID is searching for the way.",
			nl = "In een doolhof van ravijnen zoekt $USER_NAME_OR_ID de weg.",
			fr = "Dans un labyrinthe de falaises, $USER_NAME_OR_ID cherche son chemin.",
			de = "In einem Labyrinth aus Schluchten sucht $USER_NAME_OR_ID nach dem Weg.",
			it = "In un labirinto di canyon, $USER_NAME_OR_ID cerca la strada.",
			jp = "谷の迷路で $USER_NAME_OR_ID は出口を探しています。",
			pt = "Num labirinto de desfiladeiros, $USER_NAME_OR_ID procura o caminho.",
			ru = "В лабиринте каньонов $USER_NAME_OR_ID ищет дорогу.",
			es = "En un laberinto de cañones, $USER_NAME_OR_ID está buscando el camino."
		}, {
			en = "As the sun sets, $USER_NAME_OR_ID is collecting petals.",
			nl = "Terwijl de zon ondergaat, verzamelt $USER_NAME_OR_ID bloemblaadjes.",
			fr = "Alors que le soleil se couche, $USER_NAME_OR_ID récolte des pétales.",
			de = "Im Sonnenuntergang sammelt $USER_NAME_OR_ID Blütenblätter.",
			it = "Al calar del sole, $USER_NAME_OR_ID raccoglie petali.",
			jp = "夕暮れの中 $USER_NAME_OR_ID は花弁を探しています。",
			pt = "Enquanto o sol se põe, $USER_NAME_OR_ID coleciona pétalas.",
			ru = "На закате $USER_NAME_OR_ID собирает лепестки.",
			es = "Mientras que el sol se pone, $USER_NAME_OR_ID está recolectando pétalos."
		}
	},
	-- level 4
	{
		{
			en = "Alone in the darkness, $USER_NAME_OR_ID is discovering light.",
			nl = "Helemaal alleen in de duisternis ontdekt $USER_NAME_OR_ID licht.",
			fr = "Seul dans les ténèbres, $USER_NAME_OR_ID découvre la lumière.",
			de = "Alleine in der Dunkelheit entdeckt $USER_NAME_OR_ID Licht.",
			it = "Solo nel buio, $USER_NAME_OR_ID scopre la luce.",
			jp = "暗闇に一人残された $USER_NAME_OR_ID は光を求めています。",
			pt = "A sós na escuridão, $USER_NAME_OR_ID descobre a luz.",
			ru = "В одиночестве и во тьме $USER_NAME_OR_ID открывает свет.",
			es = "En la soledad de la oscuridad, $USER_NAME_OR_ID descubre la luz."
		}, {
			en = "In a land of night, $USER_NAME_OR_ID is following the streetlamps.",
			nl = "In een nachtelijk land volgt $USER_NAME_OR_ID de straatlantaarns.",
			fr = "Au pays de la nuit, $USER_NAME_OR_ID suit les réverbères.",
			de = "Im Land der Nacht folgt $USER_NAME_OR_ID den Straßenlaternen.",
			it = "Nel regno della notte, $USER_NAME_OR_ID segue i lampioni.",
			jp = "真夜中に $USER_NAME_OR_ID は街頭の明かりに沿って行きます。",
			pt = "Numa terra coberta pela noite, $USER_NAME_OR_ID segue os candeeiros de rua.",
			ru = "В ночном краю $USER_NAME_OR_ID летит от фонаря к фонарю.",
			es = "En un terreno nocturno, $USER_NAME_OR_ID sigue las farolas."
		}, {
			en = "On a spiral path, $USER_NAME_OR_ID is gathering stars.",
			nl = "Op een spiralende weg plukt $USER_NAME_OR_ID sterren.",
			fr = "Sur un chemin en spirale, $USER_NAME_OR_ID rassemble des étoiles.",
			de = "Auf einem gewundenen Pfad sammelt $USER_NAME_OR_ID Sterne.",
			it = "In un sentiero a spirale, $USER_NAME_OR_ID raccoglie stelle.",
			jp = "らせん状の道で $USER_NAME_OR_ID は星を集めています。",
			pt = "Num caminho em espiral, $USER_NAME_OR_ID reúne estrelas.",
			ru = "На спиральной тропе $USER_NAME_OR_ID собирает звезды.",
			es = "En un camino espiral, $USER_NAME_OR_ID está reuniendo a las estrellas."
		}
	},
	-- level 5
	{
		{
			en = "Amidst twisted steel, $USER_NAME_OR_ID is wandering.",
			nl = "Tussen verwrongen staal dwaalt $USER_NAME_OR_ID rond.",
			fr = "Au milieu de l'acier froid, $USER_NAME_OR_ID vagabonde.",
			de = "Umgeben von gewundenem Stahl wandert $USER_NAME_OR_ID umher.",
			it = "Tra acciaio contorto, $USER_NAME_OR_ID vaga.",
			jp = "捻じれた鉄に囲まれ $USER_NAME_OR_ID は途方に暮れています。",
			pt = "$USER_NAME_OR_ID vagueia por entre aço dobrado.",
			ru = "Среди изгибов стали $USER_NAME_OR_ID блуждает.",
			es = "En medio del retorcido metal, $USER_NAME_OR_ID vaga."
		}, {
			en = "Despite lightning and rain, $USER_NAME_OR_ID is pushing forward.",
			nl = "Ondanks regen en bliksem zet $USER_NAME_OR_ID door.",
			fr = "Malgré la pluie et les éclairs, $USER_NAME_OR_ID continue d'avancer.",
			de = "Trotz Blitz und Regen drängt $USER_NAME_OR_ID vorwärts.",
			it = "Nonostante fulmini e pioggia, $USER_NAME_OR_ID va avanti con decisione.",
			jp = "雷雨に打たれながらも $USER_NAME_OR_ID は前に進む。",
			pt = "Apesar dos relâmpagos e da chuva, $USER_NAME_OR_ID continua a avançar.",
			ru = "Грозе вопреки $USER_NAME_OR_ID стремится вперед.",
			es = "A pesar de los relámpagos y la lluvia, $USER_NAME_OR_ID sigue adelante."
		}, {
			en = "Searching in the wreckage, $USER_NAME_OR_ID is finding flowers.",
			nl = "Op zijn zoektocht in het wrak, vindt $USER_NAME_OR_ID bloemen.",
			fr = "Fouillant les décombres, $USER_NAME_OR_ID trouve des fleurs.",
			de = "Bei der Suche in Trümmern findet $USER_NAME_OR_ID Blumen.",
			it = "Frugando tra le macerie, $USER_NAME_OR_ID trova fiori.",
			jp = "がれきの中 $USER_NAME_OR_ID は花を探しています。",
			pt = "Procurando nos destroços, $USER_NAME_OR_ID encontra flores.",
			ru = "Среди обломков $USER_NAME_OR_ID находит цветы.",
			es = "Buscando en las ruinas, $USER_NAME_OR_ID encuentra flores."
		}
	},
	-- level 6
	{
		{
			en = "In a dead world, $USER_NAME_OR_ID is restoring life.",
			nl = "In een dode wereld brengt $USER_NAME_OR_ID het leven terug.",
			fr = "Dans un monde défunt, $USER_NAME_OR_ID fait renaître la vie.",
			de = "In einer toten Welt stellt $USER_NAME_OR_ID Leben wieder her.",
			it = "In un mondo morto, $USER_NAME_OR_ID riporta la vita.",
			jp = "詩の世界を $USER_NAME_OR_ID は再生させようとしています。",
			pt = "Num mundo morto, $USER_NAME_OR_ID restaura a vida.",
			ru = "Мертвому миру $USER_NAME_OR_ID возвращает жизнь.",
			es = "En un mundo moribundo, $USER_NAME_OR_ID está restaurando la vida."
		}, {
			en = "As the petals multiply, $USER_NAME_OR_ID is rediscovering beauty and balance.",
			nl = "De bloemblaadjes vermenigvuldigen zich en $USER_NAME_OR_ID ontdekt weer helemaal opnieuw schoonheid en balans.",
			fr = "Regardant les pétales se multiplier, $USER_NAME_OR_ID redécouvre la beauté et l'équilibre.",
			de = "Als sich die Blütenblätter vermehren, entdeckt $USER_NAME_OR_ID wieder Schönheit und Gleichgewicht.",
			it = "Mentre i petali si moltiplicano, $USER_NAME_OR_ID riscopre bellezza ed equilibrio.",
			jp = "花弁が増えるにつれ $USER_NAME_OR_ID は美しさと調和を再発見しています。",
			pt = "À medida que as pétalas se multiplicam, $USER_NAME_OR_ID redescobre a beleza e o equilíbrio.",
			ru = "Лепестки множатся, и $USER_NAME_OR_ID вновь открывает для себя красоту и спокойствие.",
			es = "Mientras los pétalos se multiplican, $USER_NAME_OR_ID está redescubriendo la belleza y el equilibrio."
		}, {
			en = "$USER_NAME_OR_ID is rising towards a beautiful and bright tomorrow.",
			nl = "$USER_NAME_OR_ID is op weg naar een mooie en heldere dag van morgen.",
			fr = "$USER_NAME_OR_ID se dirige vers un lendemain meilleur et rayonnant.",
			de = "$USER_NAME_OR_ID bewegt sich auf ein wunderschönes und helles Morgen zu.",
			it = "$USER_NAME_OR_ID si erge verso un domani bello e luminoso.",
			jp = "$USER_NAME_OR_ID は美しく、輝かしい明日に向かっていっています。",
			pt = "$USER_NAME_OR_ID ascende em direção a um lindo e brilhante amanhã.",
			ru = "$USER_NAME_OR_ID поднимается навстречу прекрасному и яркому завтрашнему дню.",
			es = "$USER_NAME_OR_ID se alza hacia un bello y brillante mañana."
		},
	},
	-- level 7
	{
		{
			en = "The task complete, $USER_NAME_OR_ID is drifting through memories.",
			nl = "Nu de taak gedaan is, is $USER_NAME_OR_ID in herinneringen verloren.",
			fr = "Sa tâche accomplie, $USER_NAME_OR_ID se perd dans ses souvenirs.",
			de = "Nach bewältigter Aufgabe schwelgt $USER_NAME_OR_ID in Erinnerungen.",
			it = "Terminato il compito, $USER_NAME_OR_ID si fa trasportare dai ricordi.",
			jp = "やるべきことが終わり $USER_NAME_OR_ID の記憶が走馬灯のように流れています。",
			pt = "Com a sua tarefa concluída, $USER_NAME_OR_ID anda à deriva pelas memórias.",
			ru = "Завершив дела, $USER_NAME_OR_ID заново переживает события прошлого.",
			es = "Con la tarea terminada, $USER_NAME_OR_ID se deja llevar por los recuerdos."
		}, {
			en = "Lost in thought, $USER_NAME_OR_ID is revisiting familiar landscapes.",
			nl = "In gedachten verzonken bezoekt $USER_NAME_OR_ID opnieuw bekende landschappen.",
			fr = "Perdu dans ses pensées, $USER_NAME_OR_ID retrouve des paysages familiers.",
			de = "Gedankenverloren besucht $USER_NAME_OR_ID vertraute Landschaften wieder.",
			it = "Perso nei pensieri, $USER_NAME_OR_ID rivisita paesaggi familiari.",
			jp = "物思いにふけ $USER_NAME_OR_ID は懐かしい土地を訪れています。",
			pt = "Depois de se perder nos pensamentos, $USER_NAME_OR_ID revisita paisagens familiares.",
			ru = "Погрузившись в думы, $USER_NAME_OR_ID возвращается в знакомые края.",
			es = "Perdido en los pensamientos, $USER_NAME_OR_ID visita de nuevo paisajes familiares."
		}, {
			en = "And $USER_NAME_OR_ID rested.",
			nl = "En $USER_NAME_OR_ID rustte.",
			fr = "Et $USER_NAME_OR_ID s'est reposé.",
			de = "Und $USER_NAME_OR_ID ruhte sich aus.",
			it = "E $USER_NAME_OR_ID si riposò.",
			jp = "そして $USER_NAME_OR_ID は休息を取りました。",
			pt = "E $USER_NAME_OR_ID descansou.",
			ru = "$USER_NAME_OR_ID отдыхает.",
			es = "Y $USER_NAME_OR_ID descansó."
		}
	}
}


local function GetFeedText( obj, lang )
	if obj[lang]==nil then

		if obj.en==nil then
			return obj[1]
		else
			return obj.en
		end
	else
		return obj[lang]
	end
end


function GetNumActivityFeeds( level )
	if level < #FeedText then
		return #FeedText[1 + level]
	else
		return 0
	end
end

function GetActivityFeedText( level, index, lang )

	if level < #FeedText then

		local obj = FeedText[1 + level]

		if index < #obj then

			return GetFeedText(obj[1 + index], lang)

		end
	end
	
	return nil
end

function GetCondensedActivityText( lang )

	return GetFeedText(CondensedText,lang)

end

