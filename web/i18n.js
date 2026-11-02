(() => {
  'use strict';
  const english = {
  "IRrem · ovladač": "IRrem · remote control",
  "Potvrdit změnu": "Confirm change",
  "Zrušit": "Cancel",
  "Přejít k ovladači": "Skip to remote control",
  "OVLÁDÁNÍ DOMOVA": "HOME CONTROL",
  "Hlavní navigace": "Main navigation",
  "Ovladač": "Remote",
  "Tlačítka": "Buttons",
  "Systém": "System",
  "Všechno zůstává u vás doma.": "Everything stays in your home.",
  "Připojuji…": "Connecting…",
  "01 / VIRTUÁLNÍ OVLADAČ": "01 / VIRTUAL REMOTE",
  "Světlo pod palcem": "Lighting at your fingertips",
  "Klepněte, klepněte dvakrát nebo podržte. Stejné akce jako na fyzickém ovladači.": "Tap, double-tap or hold. The same actions as on the physical remote.",
  "Virtuální tlačítka": "Virtual buttons",
  "Nejprve vytvořte tlačítko na stránce Tlačítka. IR signál není pro virtuální ovládání potřeba.": "First create a button on the Buttons page. Virtual control does not require an IR signal.",
  "Připraveno": "Ready",
  "02 / TLAČÍTKA A PŘÍKAZY": "02 / BUTTONS AND COMMANDS",
  "Každý stisk má svůj význam": "Every press has a purpose",
  "Příkazy": "Commands",
  "Název nového tlačítka": "New button name",
  "Vytvořit": "Create",
  "Hledat tlačítko…": "Search buttons…",
  "Hledat tlačítko": "Search buttons",
  "Vyberte tlačítko nebo vytvořte nové. Název, IR signál a funkce nastavíte nezávisle.": "Select a button or create a new one. Configure its name, IR signal and actions independently.",
  "Nastavení tlačítka": "Button settings",
  "Název tlačítka": "Button name",
  "Infračervený signál": "Infrared signal",
  "VOLITELNÝ": "OPTIONAL",
  "Nenapárováno": "Not paired",
  "Napárovat IR": "Pair IR",
  "Odpojit signál": "Unpair signal",
  "Uložit signál a změny": "Save signal and changes",
  "Zrušit párování": "Cancel pairing",
  "Jednoduchý stisk": "Single press",
  "Dvojitý stisk": "Double press",
  "Dlouhé podržení": "Long press",
  "Dlouhý stisk od (s)": "Long press threshold (s)",
  "Opakování DIM (s)": "DIM repeat interval (s)",
  "Dvojstisk čeká na druhé stisknutí po uvolnění prvního. Dlouhé DIM se opakuje, ostatní příkazy se při podržení spustí jednou.": "A double press waits for the second press after the first is released. Long DIM presses repeat; other commands run once while held.",
  "Uložit tlačítko": "Save button",
  "Předvolby čínských ovladačů": "Generic remote presets",
  "Přidají známé IR kódy bez přepisování vašich tlačítek. Funkce pak přiřaďte v seznamu výše.": "Add known IR codes without overwriting your buttons. Then assign commands in the list above.",
  "Předvolba ovladače": "Remote preset",
  "Přidat předvolbu": "Add preset",
  "Nový příkaz": "New command",
  "Název příkazu": "Command name",
  "Např. Bílá na čtení": "For example, Reading light",
  "Co udělat": "Action",
  "Bílá · jas a teplota": "White · brightness and temperature",
  "Barva RGB · jas a odstín": "RGB color · brightness and hue",
  "Zvýšit jas ×2, zachovat barvu": "Double brightness, preserve color",
  "Snížit jas ÷2, zachovat barvu": "Halve brightness, preserve color",
  "Vypnout napájecí relé": "Turn off the power relay",
  "Zapnout bílou 36 %": "Turn on white at 36%",
  "Bílá · zachovat jas": "White · preserve brightness",
  "Zadat univerzální kód": "Enter a universal command",
  "Teplota bílé (K)": "White temperature (K)",
  "Odstín RGB": "RGB hue",
  "Univerzální kód": "Universal command",
  "MQTT PŘÍKAZ": "MQTT COMMAND",
  "Název si zvolíte sami. Parametry příkazu Node-RED přečte automaticky; jeho kód není potřeba měnit. Úprava příkazu ovlivní všechna tlačítka, která jej používají.": "Choose your own name. Node-RED reads the command parameters automatically; its code does not need to change. Editing a command affects every button assigned to it.",
  "Uložit příkaz": "Save command",
  "Záloha tlačítek": "Button backup",
  "Všechny IR signály, názvy, příkazy, přiřazená gesta a jejich časování. Bez síťových hesel.": "All IR signals, names, commands, gesture assignments and timing settings. Excludes network passwords.",
  "Stáhnout zálohu": "Download backup",
  "Doma ve vaší síti": "At home on your network",
  "Wi-Fi síť (SSID)": "Wi-Fi network (SSID)",
  "Nové heslo Wi-Fi": "New Wi-Fi password",
  "Prázdné = ponechat uložené": "Empty = keep the saved value",
  "Smazat heslo Wi-Fi (otevřená síť)": "Clear Wi-Fi password (open network)",
  "Síťové jméno": "Hostname",
  ". Samotné irrem závisí na DNS routeru.": ". The bare hostname irrem depends on your router's DNS.",
  "Uložit Wi-Fi a restartovat": "Save Wi-Fi and restart",
  "Spojení s automatizacemi": "Connect your automations",
  "Načítám stav spojení…": "Loading connection status…",
  "Znovu připojit": "Reconnect",
  "Hlavní broker": "Primary broker",
  "Adresa / doménové jméno": "Address / hostname",
  "TLS · ověřit certifikát": "TLS · verify certificate",
  "Uživatel": "Username",
  "Nové heslo": "New password",
  "Smazat heslo hlavního brokeru": "Clear primary broker password",
  "Kořenový CA certifikát (PEM)": "Root CA certificate (PEM)",
  "Max. 2048 bajtů. TLS vyžaduje správný čas z NTP. Obvyklý port je 8883.": "Maximum 2048 bytes. TLS requires the correct time from NTP. The usual port is 8883.",
  "Záložní broker": "Backup broker",
  "Prázdné = bez záložního brokeru": "Empty = no backup broker",
  "Použije se, když selže připojení k hlavnímu.": "Used when connection to the primary broker fails.",
  "Smazat heslo záložního brokeru": "Clear backup broker password",
  "Max. 2048 bajtů. Doménové jméno musí odpovídat certifikátu.": "Maximum 2048 bytes. The hostname must match the certificate.",
  "Odesílá se do jednoho brokeru. Po přepnutí zůstane aktivní fungující server; „Znovu připojit“ začne hlavním. Home Assistant musí mít k aktivnímu brokeru přístup. Tlačítko použije uložené nastavení.": "Messages go to one broker. After failover, the working server stays active; Reconnect starts with the primary broker. Home Assistant must be able to access the active broker. The button uses the saved settings.",
  "Identita, témata a Home Assistant": "Identity, topics and Home Assistant",
  "MQTT klientské ID": "MQTT client ID",
  "Musí být jedinečné mezi klienty brokeru.": "Must be unique among the broker's clients.",
  "Základ MQTT témat": "MQTT base topic",
  "Beze změny zůstává stabilní při přejmenování zařízení.": "Remains stable when the device is renamed unless you change it here.",
  "Zpráva online obnoví Discovery. Prázdné = nesledovat.": "An online message refreshes Discovery. Empty = do not subscribe.",
  "Univerzální příkazy na /trigger, bez retain, QoS 0. Stav a dostupnost se uchovávají. Staré stisky se po výpadku nepřehrávají.": "Universal commands use /trigger, without retain, at QoS 0. State and availability are retained. Old presses are not replayed after an outage.",
  "Časování spojení a IR příjmu": "Connection and IR reception timing",
  "Opakovat připojení po (s)": "Reconnect interval (s)",
  "Timeout spojení / TLS (s)": "Connection / TLS timeout (s)",
  "Uložit MQTT a restartovat": "Save MQTT and restart",
  "05 / SYSTÉM": "05 / SYSTEM",
  "Zařízení pod kontrolou": "Your device, under control",
  "Zařízení a displej": "Device and display",
  "Název zařízení": "Device name",
  "Časový server NTP": "NTP time server",
  "Displej zapnutý po startu": "Display on at startup",
  "Displej zhasnout po nečinnosti (s)": "Turn off display after inactivity (s)",
  "0 = nezhasínat. Tlačítkem na desce jej znovu zapnete.": "0 = always on. Press the button on the board to turn it on again.",
  "Pokročilé časování IR přijímače": "Advanced IR receiver timing",
  "Vzorkování držení (ms)": "Hold sampling interval (ms)",
  "Práh dlouhého stisku patří jednotlivému tlačítku na stránce Tlačítka.": "Set each button's long-press threshold on the Buttons page.",
  "Uložit systém a restartovat": "Save system and restart",
  "Úplná záloha nastavení": "Full settings backup",
  "Tlačítka, příkazy, Wi-Fi, MQTT, displej a hesla. Soubor obsahuje přístupové údaje; uschovejte jej bezpečně. Firmware se aktualizuje samostatně níže.": "Buttons, commands, Wi-Fi, MQTT, display and passwords. The file contains credentials; store it securely. Update the firmware separately below.",
  "Stáhnout úplnou zálohu": "Download full backup",
  "Obnovit systém": "Restore system",
  "Vyberte firmware.bin pro IRrem a původní TTGO T-Display. Nastavení a tlačítka zůstanou uložené.": "Select firmware.bin for IRrem and the original TTGO T-Display. Settings and buttons will be preserved.",
  "Nahrát firmware": "Upload firmware",
  "Restartovat zařízení": "Restart device",
  "Poslední odeslané příkazy": "Recently sent commands",
  "Lokální provoz · bez cloudu": "Local operation · no cloud",
  "Zařízení neodpovědělo včas. Zkontrolujte připojení.": "The device did not respond in time. Check your connection.",
  "IR spárováno": "IR paired",
  "Pouze virtuální": "Virtual only",
  "Bílá": "White",
  "Režim ": "Mode ",
  "Podržení: ": "Hold: ",
  "Bez přiřazené akce": "No action assigned",
  "Dokončete nebo zrušte probíhající párování.": "Finish or cancel the current pairing process.",
  "Nenapárováno · virtuální ovládání funguje i bez IR": "Not paired · virtual control works without IR",
  "Dvojstisk je zapnutý: jednoduchý stisk počká na uplynutí mezery. ": "Double press is enabled: a single press waits for the double-press interval. ",
  "Dvojstisk je vypnutý: rychlé stisky se počítají jednotlivě. ": "Double press is disabled: quick presses are counted individually. ",
  "Podržení je zapnuté; krátký stisk s jinou akcí se vyhodnotí po uvolnění. Dlouhé DIM se opakuje.": "Hold is enabled; a short press with a different action is processed on release. Long DIM presses repeat.",
  "Podržení je vypnuté: i dlouhé držení znamená jediný jednoduchý stisk.": "Hold is disabled: even a long hold produces only one single press.",
  "Tlačítko uloženo.": "Button saved.",
  "Tlačítko vytvořeno. Teď můžete přiřadit funkce nebo IR signál.": "Button created. You can now assign actions or an IR signal.",
  "Smazat vybrané tlačítko včetně jeho IR signálu a přiřazení?": "Delete the selected button, including its IR signal and assignments?",
  "Tlačítko smazáno.": "Button deleted.",
  "Uvolněte ovladač a stiskněte vybrané tlačítko.": "Release the remote, then press the selected button.",
  "Vyberte znovu párování tlačítka.": "Start button pairing again.",
  "Předvolba přidána. Duplicitní signály byly přeskočeny; přiřaďte funkcím příkazy.": "Preset added. Duplicate signals were skipped; assign commands to the actions.",
  "Upravit příkaz": "Edit command",
  "Příkaz uložen. Node-RED není potřeba upravovat.": "Command saved. No changes to Node-RED are needed.",
  "Smazat příkaz? Příkaz přiřazený tlačítku smazat nelze.": "Delete this command? A command assigned to a button cannot be deleted.",
  "Příkaz smazán.": "Command deleted.",
  "Heslo je uložené.": "A password is saved.",
  "Heslo není uložené.": "No password is saved.",
  "Nastavení uloženo. Zařízení se restartuje.": "Settings saved. The device is restarting.",
  "Opětovné připojení vyžádáno. Rozepsané nastavení se neukládá.": "Reconnection requested. Unsaved settings are not saved.",
  "Záloha připravena ke stažení.": "Backup ready to download.",
  "Soubor zálohy je příliš velký.": "The backup file is too large.",
  "Tento soubor patří do obnovy ": "Use this file to restore ",
  "celého systému.": "the entire system.",
  "tlačítek.": "buttons.",
  "Obnovit {0} tlačítek a {1} příkazů? {2}": "Restore {0} buttons and {1} commands? {2}",
  "Nahradí se i síťová nastavení a zařízení se restartuje.": "Network settings will also be replaced and the device will restart.",
  "Současný seznam bude nahrazen.": "The current list will be replaced.",
  "Systém obnoven. Zařízení se restartuje.": "System restored. The device is restarting.",
  "Tlačítka a příkazy obnoveny.": "Buttons and commands restored.",
  "Nahrát {0} jako firmware IRrem? Během nahrávání neodpojujte napájení.": "Upload {0} as IRrem firmware? Keep the power connected during the upload.",
  "Nahrávám firmware…": "Uploading firmware…",
  "Spojení se přerušilo. Ověřte verzi v systému.": "The connection was interrupted. Check the version on the System page.",
  "Vypršel čas aktualizace. Ověřte verzi v systému.": "The update timed out. Check the version on the System page.",
  "Firmware ověřen. Zařízení se restartuje.": "Firmware verified. The device is restarting.",
  "Aktualizace úspěšná, probíhá restart.": "Update successful; restarting.",
  "Zařízení se restartuje.": "The device is restarting.",
  "Zařízení připojeno": "Device connected",
  "Párování IR": "IR pairing",
  "MQTT připojeno": "MQTT connected",
  "podržení": "hold",
  "opakování": "repeat",
  "Čekám na Wi-Fi": "Waiting for Wi-Fi",
  "Broker není nastaven": "No broker configured",
  "Připojeno": "Connected",
  "Čas NTP": "NTP time",
  "Synchronizován": "Synchronized",
  "Čeká": "Waiting",
  "Volná RAM": "Free RAM",
  "Přijaté IR rámce": "Received IR frames",
  "Odeslané příkazy": "Sent commands",
  "Neodeslané příkazy": "Unsent commands",
  "Poslední IR kód": "Last IR code",
  "OTA oddíl": "OTA partition",
  "Čekám na signál…": "Waiting for a signal…",
  "OTA vyžaduje první instalaci rozdělení flash přes USB.": "OTA requires an initial USB installation with a suitable flash partition layout.",
  "Zařízení je znovu online.": "The device is back online.",
  "Spojení přerušeno": "Connection lost",
  "{0} · {1} tlačítek": "{0} · {1} buttons",
  "Neplatné nebo neúplné nastavení.": "Invalid or incomplete settings.",
  "Síťové jméno: 1–32 malých písmen, číslic nebo pomlček, bez .local.": "Hostname: 1–32 lowercase letters, digits or hyphens, without .local.",
  "Zadejte DNS jméno nebo IPv4 adresu bez http:// a bez portu.": "Enter a DNS hostname or IPv4 address without http:// or a port.",
  "Záložní broker vyžaduje vyplněný hlavní broker.": "A backup broker requires a primary broker.",
  "Hlavní a záložní broker mají být různé adresy nebo porty.": "The primary and backup brokers must have different addresses or ports.",
  "Témata nesmí obsahovat mezery, +, # ani prázdné úrovně /; prefix má být bez koncového /.": "Topics must not contain spaces, +, # or empty / levels; the prefix must not end in /.",
  "Klientské ID nesmí obsahovat mezery ani řídicí znaky.": "The client ID must not contain spaces or control characters.",
  "Heslo Wi-Fi musí mít 8–63 bajtů.": "The Wi-Fi password must contain 8–63 bytes.",
  "Pro TLS vložte CA certifikát příslušného brokeru ve formátu PEM.": "For TLS, provide the broker's CA certificate in PEM format.",
  "Neplatný typ nebo příliš dlouhý text v nastavení.": "Invalid type or text too long in settings.",
  "Číselná hodnota je mimo povolený rozsah.": "A numeric value is outside the allowed range.",
  "Neplatná hodnota přepínače.": "Invalid toggle value.",
  "Neplatné heslo.": "Invalid password.",
  "Neplatný přepínač smazání hesla.": "Invalid password-clear toggle.",
  "Čekám na síť": "Waiting for the network",
  "Čas pro učení vypršel.": "Learning timed out.",
  "Signál zachycen. Zadejte jméno a uložte.": "Signal captured. Enter a name and save.",
  "IR aktivní": "IR active",
  "Poslední stisk": "Last press",
  "Poslední tlačítko": "Last button",
  "Délka držení": "Hold duration",
  "Broker neodpověděl v časovém limitu": "The broker did not respond in time",
  "Spojení s brokerem se přerušilo": "The broker connection was lost",
  "Nelze otevřít síťové spojení": "Cannot open a network connection",
  "Broker odmítl verzi MQTT": "The broker rejected the MQTT version",
  "Broker odmítl klientské ID": "The broker rejected the client ID",
  "Broker není dostupný": "The broker is unavailable",
  "Nesprávný uživatel nebo heslo": "Incorrect username or password",
  "Broker nepovolil přístup": "The broker denied access",
  "Chyba spojení": "Connection error",
  "MQTT je vypnuté v nastavení": "MQTT is disabled in settings",
  "Vyplňte adresu hlavního brokeru": "Enter the primary broker address",
  "Čekám na připojení Wi-Fi": "Waiting for a Wi-Fi connection",
  "Navazuji spojení…": "Connecting…",
  "TLS čeká na synchronizaci času NTP": "TLS is waiting for NTP time synchronization",
  "DNS / mDNS: adresu brokeru nelze přeložit": "DNS / mDNS: cannot resolve the broker address",
  "TLS spojení selhalo": "TLS connection failed",
  "Zapnout · bílá 36 %": "Turn on · white at 36%",
  "Vypnout relé": "Turn off relay",
  "Bílá %u %%": "White %u %%",
  "Teplá RGB %u %%": "Warm RGB %u %%",
  "RGB 24 tlačítek": "RGB 24 buttons",
  "RGB 24 tlačítek – starší kódy": "RGB 24 buttons – older codes",
  "RGB / teplá a studená bílá 24 tlačítek": "RGB / warm and cool white, 24 buttons",
  "RGB 44 tlačítek / DIY": "RGB 44 buttons / DIY",
  "RGBW 40 tlačítek": "RGBW 40 buttons",
  "RGB 21 tlačítek": "RGB 21 buttons",
  "Mini 9 tlačítek": "Mini 9 buttons",
  "Učící ovladač 6 tlačítek": "Learning remote, 6 buttons",
  "Neplatný nebo nekompatibilní soubor zálohy.": "Invalid or incompatible backup file.",
  "Záloha je poškozená nebo obsahuje neplatné přiřazení. Nic se nezměnilo.": "The backup is corrupt or contains an invalid assignment. Nothing was changed.",
  "Nastavení se změnilo; načtěte zálohu znovu.": "Settings have changed; load the backup again.",
  "Zálohu se nepodařilo uložit.": "Could not save the backup.",
  "Toto rozdělení flash nepodporuje OTA. Je nutná první instalace přes USB.": "This flash layout does not support OTA. An initial USB installation is required.",
  "Firmware má nesprávnou velikost.": "The firmware size is invalid.",
  "Nelze připravit OTA oddíl.": "Cannot prepare the OTA partition.",
  "Aktualizace nebyla dokončena. Původní firmware zůstává aktivní.": "The update did not complete. The original firmware remains active.",
  "Kontrola firmwaru selhala. Původní firmware zůstává aktivní.": "Firmware verification failed. The original firmware remains active.",
  "Požadavek musí přijít z webu zařízení.": "The request must come from the device's web interface.",
  "Očekávám binární soubor.": "A binary file is required.",
  "Neplatný požadavek.": "Invalid request.",
  "Nedostatek paměti.": "Not enough memory.",
  "Neúplný požadavek.": "Incomplete request.",
  "Neplatný JSON.": "Invalid JSON.",
  "Učení zrušeno.": "Learning cancelled.",
  "Neplatný virtuální stisk.": "Invalid virtual press.",
  "Tlačítko už neexistuje.": "The button no longer exists.",
  "Probíhá učení IR; ovladač je pozastaven.": "IR learning is in progress; remote control is paused.",
  "MQTT není připojeno.": "MQTT is not connected.",
  "Jiné virtuální tlačítko je stále stisknuté.": "Another virtual button is still pressed.",
  "Virtuální stisk vypršel nebo nemá platný začátek.": "The virtual press expired or has no valid starting event.",
  "Nastavení se mezitím změnilo. Obnovte seznam a zkuste to znovu.": "Settings have changed. Refresh the list and try again.",
  "Neplatný CA certifikát.": "Invalid CA certificate.",
  "Neplatný stav displeje.": "Invalid display state.",
  "Čas displeje musí být 0–3600 s.": "Display timeout must be 0–3600 s.",
  "Chybí ID tlačítka.": "Missing button ID.",
  "Tlačítko neexistuje.": "The button does not exist.",
  "Lze uložit nejvýše 64 tlačítek.": "You can save at most 64 buttons.",
  "Název musí mít 1–48 bajtů.": "The name must contain 1–48 bytes.",
  "Neplatný příkaz.": "Invalid command.",
  "Časování je mimo povolený rozsah.": "Timing is outside the allowed range.",
  "Signál vypršel; spusťte párování znovu.": "The signal expired; start pairing again.",
  "Chybí ID příkazu.": "Missing command ID.",
  "Příkaz neexistuje.": "The command does not exist.",
  "Příkaz používá tlačítko. Nejprve změňte jeho přiřazení.": "A button uses this command. Change its assignment first.",
  "Lze uložit nejvýše 64 příkazů.": "You can save at most 64 commands.",
  "Neplatný název nebo parametry příkazu.": "Invalid command name or parameters.",
  "Neplatná předvolba.": "Invalid preset.",
  "Předvolba se nevejde do 64 tlačítek. Nic se nezměnilo.": "This preset would exceed the 64-button limit. Nothing was changed.",
  "Duplicitní název / signál nebo neplatné přiřazení příkazu. Nic se nezměnilo.": "Duplicate name / signal or invalid command assignment. Nothing was changed.",
  "Nastavení se nepodařilo uložit.": "Could not save settings.",
  "Potvrdit": "Confirm",
  "Obnovit seznam": "Refresh list",
  "Smazat": "Delete",
  "Mezera dvojstisku (s)": "Double-press interval (s)",
  "Jas (%)": "Brightness (%)",
  "Obnovit ze souboru": "Restore from file",
  "MQTT zapnuto": "MQTT enabled",
  "mqtt.doma.cz nebo homeassistant.local": "mqtt.example.org or homeassistant.local",
  "DNS, mDNS (.local) nebo IPv4. Bez http:// a portu.": "DNS, mDNS (.local) or IPv4. No http:// or port.",
  "Automaticky podle MAC": "Automatic, based on MAC",
  "Automaticky irrem/irrem-<MAC>": "Automatic: irrem/irrem-<MAC>",
  "STAV": "STATE",
  "DOSTUPNOST": "AVAILABILITY",
  "Uvolnit IR po tichu (ms)": "Release IR after silence (ms)",
  "Aktualizace firmwaru · OTA": "Firmware update · OTA",
  "Soubor firmwaru": "Firmware file",
  "Diagnostika": "Diagnostics",
  "Chyba {0}": "Error {0}",
  "Bez akce": "No action",
  "— Bez akce —": "— No action —",
  "Jas +": "Brightness +",
  "Jas −": "Brightness −",
  "Dvojstisk: ": "Double press: ",
  "Seznam obnoven.": "List refreshed.",
  "Aktualizace selhala.": "Update failed.",
  "MQTT odpojeno": "MQTT disconnected",
  "stisk": "press",
  "dvojstisk": "double press",
  "bez IP": "no IP",
  "Odpojeno": "Disconnected",
  "Nenalezeno": "Not found",
  "Zapnout": "On",
  "Vypnout": "Off",
  "Jazyk / Language": "Language / Jazyk"
};
  const storageKey = 'irrem.ui-language';
  let language = navigator.language.toLowerCase().startsWith('cs') ? 'cs' : 'en';
  try {
    const saved = localStorage.getItem(storageKey);
    if (saved === 'cs' || saved === 'en') language = saved;
  } catch { /* Storage is optional; the current visit still supports both languages. */ }

  const bindings = new Map();
  function t(key, ...values) {
    if (key == null) return '';
    const translated = language === 'en' ? english[key] ?? key : key;
    if (!values.length) return String(translated);
    return String(translated).replace(/\{(\d+)\}/g, (_, index) => String(values[Number(index)] ?? ''));
  }
  function message(value) {
    const original = Object.keys(english).find(key => english[key] === value) ?? value;
    return t(original);
  }
  function bind(node, property, render) {
    let properties = bindings.get(node);
    if (!properties) { properties = new Map(); bindings.set(node, properties); }
    properties.set(property, render);
    render();
  }
  function bindText(node, render) {
    const value = render();
    node.textContent = value;
    // Empty helper elements become containers; never erase their later children.
    if (value === '') { bindings.get(node)?.delete('textContent'); return value; }
    bind(node, 'textContent', () => { node.textContent = render(); });
    return value;
  }
  function option(render, value) {
    const element = new Option('', value);
    bindText(element, render);
    return element;
  }
  function prune() {
    for (const node of bindings.keys()) if (!node.isConnected) bindings.delete(node);
  }
  // Only the original static markup is scanned. Dynamic user names are never scanned.
  const walker = document.createTreeWalker(document.documentElement, NodeFilter.SHOW_TEXT);
  const originalNodes = [];
  while (walker.nextNode()) originalNodes.push(walker.currentNode);
  for (const node of originalNodes) {
    if (node.parentElement?.closest('script, style, textarea')) continue;
    const source = node.nodeValue, key = source.trim();
    if (Object.hasOwn(english, key)) {
      bind(node, 'nodeValue', () => { node.nodeValue = source.replace(key, () => t(key)); });
    }
  }
  for (const element of document.querySelectorAll('[placeholder], [aria-label], [title]')) {
    for (const attribute of ['placeholder', 'aria-label', 'title']) {
      const source = element.getAttribute(attribute);
      if (source && Object.hasOwn(english, source)) {
        bind(element, attribute, () => element.setAttribute(attribute, t(source)));
      }
    }
  }
  const selector = document.getElementById('ui-language');
  function selectLanguage(value) {
    if (value !== 'en' && value !== 'cs') return;
    language = value;
    document.documentElement.lang = value;
    selector.value = value;
    try { localStorage.setItem(storageKey, value); } catch { /* Keep the in-memory choice. */ }
    prune();
    for (const properties of bindings.values()) for (const render of properties.values()) render();
    document.dispatchEvent(new Event('irrem-languagechange'));
  }
  window.IRremI18n = { t, message, bindText, option, prune, get locale() { return language === 'cs' ? 'cs-CZ' : 'en-US'; } };
  selector.addEventListener('change', () => selectLanguage(selector.value));
  selectLanguage(language);
})();
