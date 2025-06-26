
# OSPF by Julien et Romain

  

## Sommaire

- Généralités 

- Fonctionnement

- Fonctionnalités

- Points fort et faibles

- Résultats des tests

- Synthèse

-----

  

## Généralités 

Le protocole a pour nom humoristique `OSPF à la sauce Julien et Romain`.

Il a été entièrement fait en C++.

La philosophie du protocole est d'envoyer le moins possible sur le réseau et de faire le plus grand travaille en interne du routeur.

Notre protocole fixe tout d'abord les hostname des routeurs à R suivi d'un nombre en 0 et 999999.

```

R0 (valide)

R32 (valide)

Router_4 (invalide)

```

Sachant qu'il doit obligatoirement y avoir un routeur R0 qui sera le routeur `default originate` qui aura pour rôle d'être le routeur par défaut de notre système de routage.  

## Fonctionnement

L’échange d’informations repose sur un seul type de message.
Chaque message contient les éléments suivants :

- Le nom du routeur (hostname),

- Son adresse IP et son masque de sous-réseau (pour identifier l’adresse réseau),

- Le coût du lien (valeur associée à la qualité ou au coût d’un lien réseau),

- Un timestamp correspondant à la date de création du message.

#### Utilisation du timestamp

Chaque routeur s’annonce périodiquement en générant un message avec un timestamp unique et croissant.
Ce timestamp permet aux autres routeurs de déterminer la fraîcheur de l'information. Ainsi :

- Si un message avec un coût de lien mis à jour est reçu, il remplace l’ancien dans la base de données locale du routeur destinataire.

- Si le timestamp est plus récent mais que l'information ne change pas, seul le timestamp est mis à jour localement. 

#### Diffusion des informations
Le protocole repose sur une diffusion régulière :
À chaque itération, un routeur envoie tous les enregistrements qu’il possède à tous ses voisins.

Cependant, il existe une subtilité :

- Si un routeur reçoit une information concernant ses propres sous-réseaux, il met à jour les timestamps de ces enregistrements avant de les renvoyer, assurant ainsi une propagation plus fraîche de l'information.

#### Expiration des données

Grâce aux timestamps constamment mis à jour, si un enregistrement devient trop ancien (par exemple si un routeur tombe ou qu’un lien est défaillant), il sera automatiquement supprimé par les routeurs qui suivent le protocole.
Cela permet une auto-épuration des données obsolètes, sans nécessiter de messages de suppression explicites.


## Fonctionnalités
- Possibilité de choisir sur quels interfaces sont inclus dans le protocole.
- Détecter si un lien ne fonctionne plus. (Tolérance aux pannes)
- Mettre à jour la basse interne qui contient la topologie du réseau.
- Calculer le chemin optimal pour atteindre un réseau.
- Possibilité de lister les routeurs voisins.

### Fonctionnalités pas encore implémenté
- Possibilité de choisir un routeur comme routeur par défaut (Pas fait)
- La reception de paquets modifiant a topologie du réseau fait temporairement diminuer l'intervale d'envois des paquets.


## Point forts et faibles

### fiabité
Le protocole repose sur **UDP**, donc il n’y a **aucune garantie de réception** des paquets intègres. Cependant, notre code est capable, dans une certaine mesure, de **détecter les paquets mal formés**, qui seront alors ignorés.
De plus, **le protocole renvoie plusieurs fois les mêmes informations**, ce qui rend la probabilité que **six paquets successifs soient perdus** extrêmement faible.


### Sécurité
**Aucun mécanisme** de sécurité n’a été mis en place.
Les messages sont envoyés **en clair sur le réseau**. Un attaquant connaissant le protocole pourrait forger ses propres trames et faire croire à tous les routeurs dits "fiables" de fausses informations.
Il pourrait également **rendre le réseau indisponible** en modifiant les tables de routage pour qu’elles soient invalides, ou en **saturant les routeurs** avec des enregistrements "dibon" (fictifs), ce qui ralentirait considérablement le réseau.



### Robustesse
Le code a été conçu selon le principe de `zero trust`, ce qui permet d’avoir des fonctions qui ne lancent aucune **exception non contrôlée**.
Grâce à des tests unitaires (décrits plus en détail plus loin), nous avons pu valider que le code se comporte correctement, même en présence de cas inattendus.
Ces tests garantissent que les fonctions fonctionnent aussi bien dans les cas **normaux** que dans les cas **erronés**.

> Une anecdote illustre bien la robustesse du code :
Lors de la création des tests unitaires, plusieurs bases d’états des liens ont été créées. À un moment, une **inattention dans le nommage des variables** a provoqué une incohérence : certains routeurs simulés étaient reliés à des réseaux qu’ils n’étaient pas censés connaître.
Malgré cela, **le protocole n’a provoqué aucun crash**. Il s’est simplement mis à essayer d’envoyer des paquets vers des réseaux inexistants dans la configuration actuelle, ce qui montre sa capacité à gérer des erreurs de manière contrôlée.




## Résultats des tests
Pour tester notre protocole, des tests unitaires ont été faits pour garantir le fonctionnement.

Dans le répertoire logic le fichier unit_test.cpp contient tous les tests qui garantissent un fonctionnement dans les cas extrêmes.

Les tests unitaires sont lancé avec les commandes suivantes
```bash
./compile
./unit_test
[Tout les tests]
```
Ces tests unitaires test les fonctionnalités en faisant abstraction du transfert réseau. (La démonstration qui montre les echange réseaux a été faite)

#### Résumé des tests:
- Validation du nom du routeur
- Validation des CIDR reçu
- Test de sérialisation et dé-sérialisation des messages
- Test d'affichage de basse locale en injectant directement les données dans la structure.
- Test d'ajout depuis la fonction côté serveur.
-  Test d'ajout d'un même enregistrement avec un cout différent
- Test de mise à jour du timestamp
- Test de l'autosuppression des enregistrements trop vieux.
- Test si trois routeurs peuvent converger (les messages ne sont pas envoyés, seuls les structure sont échangées dans le code)
- Test de conversion IP du routeur + masque de sous-réseau vers l'IP du sous-réseau.
- Test d'extraction de tous les réseaux existants depuis un routeur.
- Test d'extraction du nom de tous les routeurs existants depuis un routeur.
- Test de la création d'une matrice depuis la basse de donner locale.
- Test de l'algorithme de sélection du chemin optimal.
- Test d'affichage de la liste des voisins.


## Synthèse
Points forts :
- **Simplicité** : un seul type de message à gérer réduit la complexité de l’implémentation.

- **Diffusion implicite** : les données circulent sans avoir besoin d’arbres ou de chemins spécifiques.

- **Actualisation automatique** : le système se met à jour en continu tant que les messages sont reçus.

- **Résilience aux pannes** : un routeur inactif est détecté implicitement grâce à l’expiration des timestamps.

Points faibles :
- **Pas d’optimisation de bande passante** : tous les enregistrements sont envoyés à chaque itération, ce qui peut être coûteux dans les grands réseaux.

- **Propagation lente** en cas de lien faible ou congestionné.

- **Pas de protection** contre les boucles ou doublons si deux routeurs mettent à jour les mêmes enregistrements sans coordination.

- **Pas de mécanisme de vérification d’authenticité** ou d’intégrité, ce qui est un vrai souci du côté sécurité.
