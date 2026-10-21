-------------------------------------------------------------------------------
-- Notes:  A 2D table which is indexed on string ID and language.  Also
--         provides an accessor to the table.
--
-- Author: John Edwards
-- Date C: 11-10-2008
-------------------------------------------------------------------------------

function GetLocalizedString( id )
	local strTbl = LocalizedStrings[ id ]
	if not strTbl then
		error( "No string with id: "..id )
	end

	-- JP wants Japanese or English
	-- AS wants English, Korean and Chinese Simplified/Traditional
	-- CH wants Chinese Simplified/Traditional
	-- UA wants English, French, Portuguese, Spanish
	-- EU wants everything except the Japanese, Korean and Chinese Simplified/Traditional
	local languageName = SystemLanguageName()
	if ("JP" == REGION) then
		if (languageName == "english") then

		else
			languageName = "japanese"
		end
	elseif ("AS" == REGION) then
		if (languageName == "korean") or (languageName == "chinese_t")  then

		else
			languageName = "english"
		end
	elseif ("CH" == REGION) then
		if (languageName == "chinese_t") or (languageName == "chinese_s")  then

		else
			languageName = "english"
		end
	elseif ("UA" == REGION) then
		if (languageName == "french") or (languageName == "portuguesebr") or (languageName == "spanishla")  then
			
		else
			languageName = "english"		
		end
	else -- "EU" == REGION
		if (languageName == "japanese") or (languageName == "korean") or (languageName == "chinese_t") or (languageName == "chinese_s")  or (languageName == "portuguesebr") or (languageName == "spanishla") then
			languageName = "english"
		end
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
  { japanese   = "風ノ旅ビトのセーブデータの読み込みに失敗しました。\n続行するとデータが上書きされます。\nセーブデータの読み込みをやり直しますか？"
  , english    = "There was an error loading the Journey™ save data\nand this data will be overwritten if you proceed.\nWould you like to retry loading the save data?"
  , french     = "Une erreur s'est produite lors du chargement des données de sauvegarde de Journey™.\nCes données seront écrasées si vous continuez.\nVoulez-vous réessayer de télécharger les données de sauvegarde?"
  , spanish    = "Se produjo un error al cargar los datos guardados de Journey™\ny se sobrescribirán estos datos si continúas.\n¿Quieres volver a intentar cargar los datos guardados?"
  , german     = "Beim Laden der Journey™-Speicherdatei ist ein Fehler aufgetreten.\nDiese Datei wird überschrieben, wenn Sie fortfahren.\nMöchten Sie versuchen, die Speicherdatei erneut zu laden?"
  , italian    = "Si è verificato un errore durante il caricamento dei dati salvati di Journey™.\nProseguendo, tali dati verranno sovrascritti.\nVuoi provare a ricaricare i dati salvati?"
  , dutch      = "Er is een fout opgetreden tijdens het laden van opgeslagen gegevens van Journey™.\nAls je doorgaat, worden deze gegevens overschreven.\nWil je de opgeslagen gegevens opnieuw proberen te laden?"
  , portuguese = "Ocorreu um erro ao carregar os dados guardados de Journey™ \ne estes dados serão substituídos se continuar.\nQuer tentar carregar os dados guardados novamente?"
  , russian    = "При загрузке сохраненных данных игры \"Путешествие™\" произошла ошибка.\nЕсли продожить игру, они будут удалены.\nПовторить загрузку файла?"
  , korean     = "Journey™의 저장 데이터를 불러오는 도중에 오류가\n 발생해 계속 진행하면 데이터를 덮어쓰게 됩니다.\n저장 데이터 불러오기를 다시 시도하시겠습니까?"
  , chinese_t  = "載入Journey™保存資料時發生錯誤。\n若要繼續，您的個人檔案將會被覆寫。\n確定要重新載入保存資料嗎？"
  , chinese_s  = "載入Journey™保存資料時發生錯誤。\n若要繼續，您的個人檔案將會被覆寫。\n確定要重新載入保存資料嗎？"
  , finnish    = "Journey™-pelin tallennustietojen lataamisessa tapahtui virhe\nja tiedot korvataan, jos jatkat.\nHaluatko yrittää tallennettujen tietojen lataamista uudelleen?"
  , swedish    = "Det uppstod ett fel när sparfilen för Journey™ skulle laddas.\nFilen kommer att skrivas över om du väljer att fortsätta.\nVill du prova att ladda den igen?"
  , danish     = "Der opstod en fejl ved indlæsning af gemte data til Journey™,\nog disse data vil blive overskrevede, hvis du fortsætter.\nVil du forsøge at indlæse de gemte data igen?"
  , norwegian  = "Det oppsto en feil under innlasting av lagrete data for Journey™, \nog disse dataene vil bli overskrevet dersom du fortsetter. \nVil du forsøke å laste inn de lagrete dataene på nytt?"
  , polish     = "Wystąpił błąd wczytywania danych zapisu gry Podróż™.\nKontynuacja oznacza nadpisanie danych.\nChcesz ponowić próbę wczytania danych zapisu?"
  , turkish    = "There was an error loading the Journey™ save data\nand this data will be overwritten if you proceed.\nWould you like to retry loading the save data?"
  , arabic     = "There was an error loading the Journey™ save data\nand this data will be overwritten if you proceed.\nWould you like to retry loading the save data?"
  , portuguesebr = "Ocorreu um erro ao carregar os dados guardados de Journey™ \ne estes dados serão substituídos se continuar.\nQuer tentar carregar os dados guardados novamente?"
  , spanishla  = "Se produjo un error al cargar los datos guardados de Journey™\ny se sobrescribirán estos datos si continúas.\n¿Quieres volver a intentar cargar los datos guardados?"
  }
, GameName =
  { japanese   = "風ノ旅ビト"
  , english    = "Journey"
  , french     = "Journey"
  , spanish    = "Journey"
  , german     = "Journey"
  , italian    = "Journey"
  , dutch      = "Journey"
  , portuguese = "Journey"
  , russian    = "Путешествие"
  , korean     = "Journey"
  , chinese_t  = "Journey"
  , chinese_s  = "Journey"
  , finnish    = "Journey"
  , swedish    = "Journey"
  , danish     = "Journey"
  , norwegian  = "Journey"
  , polish     = "Podróż"
  , turkish    = "yolculuk"
  , arabic     = "Journey"	
  , portuguesebr = "Journey"
  , spanishla  = "Journey"
  }
, NonOwnedSaveData =
  { japanese   = "このセーブデータは他のユーザーのセーブデータです。\nトロフィーを取得することはできず、セーブも行えません。"
  , english    = "You are not the owner of the current save data.\nYou cannot earn Trophies or save progress."
  , french     = "Vous ne possédez pas les données sauvegardées sélectionnées.\nVous ne pouvez ni remporter de trophées ni sauvegarder votre progression."
  , spanish    = "No eres el propietario de estos datos guardados.\nNo puedes obtener Trofeos ni guardar tus avances."
  , german     = "Sie sind nicht der Besitzer der aktuellen Speicherdatei.\nSie können keine Trophäen verdienen oder Fortschritte speichern."
  , italian    = "I dati salvati correnti non ti appartengono.\nNon potrai salvare i tuoi progressi né guadagnare Trofei."
  , dutch      = "Je bent niet de eigenaar van de huidige opgeslagen gegevens.\nJe kunt geen trofeeën verdienen of voortgang opslaan."
  , portuguese = "Não é o proprietário dos actuais dados guardados.\nNão pode ganhar Troféus nem guardar os progressos."
  , russian    = "Эти призы принадлежат другому пользователю.\nВы не сможете получать призы и сохранять игру."
  , korean     = "현재 저장 데이터의 소유자가 아닙니다.\n트로피를 획득하거나 진행상황을 저장할 수 없습니다."
  , chinese_t  = "已載入其他使用者的保存資料。\n將無法取得獎盃或保存進度。"
  , chinese_s  = "已載入其他使用者的保存資料。\n將無法取得獎盃或保存進度。"
  , finnish    = "Et ole nykyisten tallennustietojen omistaja.\nEt voi ansaita trophyja tai tallentaa edistymistä."
  , swedish    = "Du äger inte denna sparfil.\nDu kan inte få några trophies eller spara dina framsteg."
  , danish     = "Du ejer ikke de aktuelle gemte data.\nDu kan ikke opnå trophies eller gemme dine fremskridt."
  , norwegian  = "Det er ikke du som eier de gjeldende lagrete dataene.\nDu kan ikke oppnå trofeer eller lagre fremgangen din."
  , polish     = "Nie jesteś właścicielem obecnie wybranych danych zapisu.\nNie możesz zdobywać trofeów, ani zapisywać postępów."
  , turkish    = "Kaydedebilir güncel veriler sahibi değildir.\nSiz Kupalar kazanmak ya da ilerleme kaydedilemiyor."
  , arabic     = "You are not the owner of the current save data.\nYou cannot earn Trophies or save progress."
  , portuguesebr = "Não é o proprietário dos actuais dados guardados.\nNão pode ganhar Troféus nem guardar os progressos."
  , spanishla    = "No eres el propietario de estos datos guardados.\nNo puedes obtener Trofeos ni guardar tus avances."
  }
, SaveDataDesc =
  { japanese   = "セーブデータ"
  , english    = "Save Data"
  , french     = "Données de sauvegarde"
  , spanish    = "Datos guardados"
  , german     = "Speicherdaten"
  , italian    = "Dati salvati"
  , dutch      = "Opgeslagen gegevens"
  , portuguese = "Dados guardados"
  , russian    = "Сохраненные данные"
  , korean     = "데이터 저장"
  , chinese_t  = "保存資料"
  , chinese_s  = "风之旅人"
  , finnish    = "Tallennustiedot"
  , swedish    = "Sparade data"
  , danish     = "Gemte data"
  , norwegian  = "Lagrede data"
  , polish     = "Dane zapisu"
  , turkish    = "kaydet Verileri"
  , arabic     = "Save Data"
  , portuguesebr = "Dados guardados"
  , spanishla  = "Datos guardados"
  }
}
