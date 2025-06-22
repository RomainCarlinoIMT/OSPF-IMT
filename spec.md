
# OSPF by Julien et Romain

  

## Sommaire

- Généralités et fonctionnement

- Fonctionnalités

- Résultats des tests

-----

  

## Généralités et fonctionnement

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

L'échange d'information repose sur un seul type de message. Chaque message contient le nom d'un routeur qui est sont hostname. Son ip dans le réseau avec son masque (ce qui permet d'avoir l'IP du routeur dans le réseau et l'adresse du réseau). Le cout du lien. Et une information très importante, le timestamp que création du message.  

Grâce aux timestamp générer par le routeur qui s'annonce tout le monde peut connaître la fraicheur d'un enregistrement. Le but de chacun de paquets est d'être bien sûr d'être envoyé à tout le monde. 

Sachant que pour la création des paquets intuitivement chaque routeur ne crée que les paquets qui le concerne. À chaque itération, les routeurs envoient toutes ses enregistrements à tous ses voisins. Mais la subtilité et que si le paquet concerne ses sous-réseaux, alors, il mettra à jour les timestamp de ses enregistrements.

Dans le cas où des informations changent par exemple le cout d'un lien, le timestamp sert à dire au récepteur de ne plus prendre en compte l'ancienne valeur et d'utiliser la nouvelle valeur. Si aucune information ne change, mais que le timestamp est plus grand, alors on met, on met à jour le timestamp dans la basse du routeur qui a reçu le paquet.

Le fait que les timestamp soit constamment mis à jour si l'un d'eux vient à devenir trop vieux, car un routeur est tombé ou qu'un lien est devenu défectueux.  L'ensemble des routeurs qui suivent le protocole vont automatiquement supprimer les enregistrements trop vieux.

## Fonctionnalités
- Possibilité de choisir sur quels réseaux sont inclus dans le protocole.
- Détecter si un lien ne fonctionne plus. (Tolérance aux pannes)
- Mettre à jour la basse interne qui contient la topologie du réseau.
- Calculer le chemin optimal pour atteindre un réseau.
- Possibilité de lister les routeurs voisins (Pas fait)
- Possibilité de choisir un routeur comme routeur par défaut (Pas fait)

## Résultats des tests
Pour tester notre protocole, des tests unitaires ont été faits pour garantir le fonctionnement.

Dans le répertoire logic le fichier unit_test.cpp contient tous les tests qui garantissent un fonctionnement dans les cas extrêmes.

Les tests unitaires sont lancé avec les commandes suivantes
```bash
./compile
./unit_test
[Tout les tests]
```
Ces tests unitaires test les fonctionnalités en faisant abstraction du transfert réseau. (Une démonstration sera probablement faite pour prouver le transfert des données sur le réseau.)

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

/!\ Tests pas encore réalisés /!\