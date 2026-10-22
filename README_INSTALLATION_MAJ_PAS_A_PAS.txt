ACID303 SOLO V0.30.0 - INSTALLATEUR ET NOTIFICATION DES MISES A JOUR
===================================================================

IMPORTANT : ce ZIP contient un projet C++ complet et un projet d'INSTALLATEUR.
Il ne peut pas encore contenir un Setup.exe fonctionnel : pour cela, vous devez
compiler le VST3 x64 sous Windows et ensuite compiler le script Inno Setup.

OBJECTIF
--------
- Au premier lancement, installer le VST3 sans copier-coller manuel.
- Dossier propose : C:\Program Files\Common Files\VST3
- L'utilisateur peut choisir un autre dossier.
- Les prochaines mises a jour reutilisent le precedent dossier d'installation.
- Aucun fichier dans Documents\Acid303\midi, presets ou skins n'est remplace.
- Le VST signale une version plus recente et ouvre une page de telechargement OneDrive.
- Les utilisateurs n'ont besoin NI de client OneDrive NI de compte Microsoft,
  si le lien OneDrive est public en lecture et telechargement.

ETAPE A : CONFIGURER LA PUBLICATION (UNE FOIS)
---------------------------------------------
1. Creez un dossier de publication dans VOTRE OneDrive. Exemple :
   OneDrive\Acid303-Releases
2. Sur le dossier OneDrive : Partager > Toute personne possedant le lien >
   PEUT CONSULTER (et TELECHARGER), pas MODIFIER. Eviter date d'expiration.
3. Copiez le lien du dossier, ouvrez-le dans une fenetre de navigation privee
   et verifiez que le telechargement fonctionne SANS connexion Microsoft.
4. Choisissez une adresse HTTPS stable capable de servir un petit JSON brut :
   - une page/fichier public de votre hebergement web ; ou
   - GitHub Pages ; ou un autre hebergeur de fichiers STATIQUES.
5. Vous y publierez le fichier Publication\latest.json fourni.
   ATTENTION : le lien OneDrive standard affiche une page HTML, pas le JSON brut.
   Ne pas l'utiliser comme adresse du manifeste sans verifier qu'il sert le JSON.
6. Editez Source\UpdateConfig.h (AVANT de compiler le VST3) :
   remplacer https://VOTRE-ADRESSE-HTTPS/latest.json
   par l'URL HTTPS DIRECTE et PUBLIQUE du futur latest.json.
   Cette URL restera identique pour toutes les versions.
7. Dans Publication\latest.json, remplacez le champ download_url par le
   lien OneDrive public du dossier. Conservez-le identique pour les futures versions.

ETAPE B : CREER LE PREMIER SETUP.EXE
------------------------------------
1. Sur Windows, installer Visual Studio 2022 avec C++, Git, CMake.
2. Installer Inno Setup 6 : https://jrsoftware.org/isinfo.php
3. Decompresser le ZIP et ouvrir le dossier Acid303_Solo_V30.
4. Double-cliquer BUILD_WINDOWS.bat et attendre COMPILATION TERMINEE.
5. Double-cliquer Installeur\CREER_INSTALLATEUR.bat.
6. Le programme cree :
   Installeur\Output\Acid303Solo_Setup_0.30.0.exe
7. Fermer FL Studio puis TESTER cet EXE sur votre PC.
   Il propose C:\Program Files\Common Files\VST3.
   Si vous choisissez un autre dossier, la prochaine mise a jour le reutilise.
8. Verifier Acid303 dans FL Studio et tester 2 ou 3 presets, MIDI, skins.
9. Copier l'EXE final dans votre dossier OneDrive public.
10. Mettre le fichier latest.json a disposition sur son URL publique HTTPS.

ETAPE C : PUBLIER LA MISE A JOUR v0.31.0 (EXEMPLE)
-------------------------------------------------
1. Modifier le code et terminer vos tests.
2. DANS LE NOUVEAU PROJET, changer le numero de version :
   - CMakeLists.txt : project(Acid303Solo VERSION 0.31.0 ...)
   - Source\WebUpdate.h : installedVersion() -> "0.31.0"
   - Installeur\Acid303Solo.iss : #define MyAppVersion "0.31.0"
   - Installeur\CREER_INSTALLATEUR.bat : nom du fichier EXE en fin de script.
   Ne changez PAS AppId dans le script Inno Setup.
3. Conserver la MEME URL dans Source\UpdateConfig.h.
4. Compiler le VST3 via BUILD_WINDOWS.bat.
5. Executer Installeur\CREER_INSTALLATEUR.bat.
6. Tester Acid303Solo_Setup_0.31.0.exe sur votre propre PC.
7. Copier cet EXE dans VOTRE dossier OneDrive public.
   Attendre la fin de synchronisation / disponibilite du fichier dans le navigateur.
8. Editer le fichier latest.json sur votre hebergement public :
   "version": "0.31.0"
   "download_url": "VOTRE_LIEN_DE_DOSSIER_ONEDRIVE_PUBLIC_INCHANGE"
   "notes": "Corrections et nouveautes de la v0.31.0"
9. PUBLIER latest.json EN DERNIER, une fois le Setup.exe telechargeable.
10. Les utilisateurs ayant une v0.30.0 active voient "MAJ v0.31.0 DISPONIBLE"
    a l'ouverture de l'interface, puis environ une fois par heure.
11. En cliquant, le navigateur ouvre le dossier OneDrive public. L'utilisateur
    telecharge et lance le dernier installateur .exe.
12. Il ferme FL Studio avant l'installation. Un compte administrateur peut
    etre necessaire si le VST3 se trouve dans Program Files.
13. Il relance FL Studio apres l'installation.

INFORMATIONS IMPORTANTES
------------------------
- Un lien de partage OneDrive public n'exige pas de compte Microsoft, SI
  votre configuration OneDrive autorise "Toute personne possedant le lien".
- L'URL du JSON public sert UNIQUEMENT a detecter une nouvelle version.
- L'URL OneDrive sert UNIQUEMENT a ouvrir le dossier de telechargement.
- Pas de mise a jour automatique ou silencieuse et pas de telechargement
  d'un exe par le plugin : l'utilisateur garde le controle.
- Le code ne contient actuellement AUCUNE vraie URL de publication. Tant que
  Source\UpdateConfig.h contient le lien exemple, aucune notification ne marche.
- Tester la compatibilite du setup et la fermeture du plugin sous Windows AVANT
  distribution. Ce projet n'a pas ete compile en installateur EXE dans cet environnement.
- Pour des fichiers .exe publics, une signature Authenticode peut reduire
  certains avertissements de securite Windows ; elle ne les elimine pas tous.
- Le VST3 n'est pas un programme autonome : son dossier .vst3 est copie sous
  le dossier VST3 choisi et charge par FL Studio.

RACCOURCI POUR GENERER L'ANNONCE SANS EDITER LE JSON
---------------------------------------------------
Apres avoir copie et teste le nouvel EXE dans OneDrive, lancez :
Publication\GENERER_ANNONCE_MAJ.bat
Saisissez la nouvelle version, le lien public OneDrive et une description.
Le fichier Publication\latest.json est cree/actualise.
Il faut ENCORE publier ce fichier sur votre adresse HTTPS publique stable.
