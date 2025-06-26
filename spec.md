
  
  
  

# OSPF par Julien et Romain

  

## Sommaire

  

* Généralités

  

* Fonctionnement Détaillé et Choix Architecturaux

  

* Fonctionnalités

  

* Points forts et faibles

  

* Résultats des tests

  

* Démonstration et Résultats Visuels

  

* Synthèse

  

* Architecture Technique et Implémentation

  

## Généralités

  

Le protocole que nous avons développé, avec le nom humoristique **OSPF à la sauce Julien et Romain**, est une implémentation réseau conçue pour le routage dynamique. Il a été entièrement développé en **C++**.

  

La philosophie derrière ce protocole est d'**envoyer le moins possible d'informations** sur le réseau afin de minimiser la consommation de bande passante, et de transférer la majeure partie du **travail de traitement en interne**, au niveau de chaque routeur. Cela a pour but de décharger le réseau et à optimiser les ressources de calcul des routeurs.

  

Notre protocole impose une convention de nommage stricte pour les routeurs : chaque nom d'hôte (hostname) doit commencer par la lettre `R`, suivie d'un nombre entier compris entre 0 et 999999.

  

Exemples valides :

  
  

```

R0 R32

```

  

Exemple invalide :

  

```

Router_4

```

  

Il est obligatoire qu'un routeur soit nommé **R0**. Ce routeur particulier agit comme le "default originate router" (routeur d'origine par défaut), assumant le rôle de passerelle par défaut pour l'ensemble de notre système de routage.

  

En termes de communication, le protocole utilise le protocole de transport **UDP (User Datagram Protocol)**. Ce choix a été fait exprès et est motivé par le besoin d'une faible latence et d'une surcharge minimale pour les échanges d'informations de routage périodiques. Les messages sont envoyés sur un port UDP spécifique choisi pour cette application.

  

## Fonctionnement Détaillé et Choix Architecturaux

  

L’échange d’informations entre les routeurs repose sur un **seul et unique type de message**. Cette simplicité est la base de notre conception, réduisant la complexité de l'analyse et du traitement des messages. Chaque message encapsule les informations essentielles d'une déclaration de routeur, contenues dans une structure `RouterDeclaration` (visible dans le code C++ fourni) qui comprend les éléments suivants :

  

*  **Le nom du routeur (hostname)** : Identifiant unique du routeur déclarant.

  

*  **Son adresse IP et son masque de sous-réseau (`ip_with_mask`)** : Permet d'identifier le sous-réseau spécifique auquel le routeur est connecté, par exemple `192.168.1.0/24`.

  

*  **Le coût du lien (`link_cost`)** : Une valeur entière associée à la qualité ou au "coût" d'utilisation d'un lien réseau. Ce coût est crucial pour le calcul du chemin optimal.

  

*  **Un timestamp (`timestamp`)** : Un horodatage correspondant à la date de création ou de dernière mise à jour du message, exprimé en millisecondes depuis l'époque.

  

### Format des Données et Transfert

  

Chaque `RouterDeclaration` est sérialisée dans un format texte simple pour être transmise sur le réseau. Le format choisi est une chaîne de caractères structurée :

`{type_de_message,nom_routeur,adresse_ip_masque,coût_lien,timestamp}`

  

Par exemple : `{1,R123,10.0.1.1/24,5,1700000000000}`.

Le `1` initial sert d'identifiant pour ce type unique de message de déclaration, permettant une extensibilité future si d'autres types de messages devaient être introduits.

  

La fonction `serialize_router_definition` transforme une structure `RouterDeclaration` en cette chaîne, et `deserialize_router_definition` effectue l'opération inverse. Cette approche simple  facilite le débogage et l'interopérabilité (bien que notre protocole soit propriétaire).

  

### Utilisation du Timestamp pour la Fraîcheur des Informations

  

Le `timestamp` est un élément central pour la gestion de la fraîcheur des informations et la robustesse du protocole. Chaque routeur s’annonce périodiquement en générant un message avec un timestamp **unique et croissant**. Ce timestamp permet aux autres routeurs de déterminer la validité et la pertinence de l'information reçue :

  

*  **Mise à jour d'un lien existant** : Si un message est reçu pour un lien déjà connu, et que son `timestamp` est **plus récent**, l'entrée est mise à jour dans la base de données locale du routeur destinataire, même si le coût ou l'IP n'a pas changé (seul le timestamp est mis à jour dans ce cas). Si le coût a changé, l'ancienne entrée est remplacée.

  

*  **Détection des informations obsolètes (`cleanup_old_declarations`)** : Grâce à cette mise à jour constante des timestamps, si un enregistrement devient trop ancien (par exemple, si un routeur tombe en panne, un lien est cassé, ou un message est perdu pendant une longue période), il est automatiquement supprimé par les routeurs. Ce mécanisme d'**auto-épuration des données obsolètes** évite l'encombrement de la base de données sans nécessiter de messages de suppression explicites (contrairement à d'autres protocoles qui utilisent des messages de `LSA Age` ou `Flush`).

  

### Diffusion des Informations (Gossip Protocol Like)

  

Le protocole repose sur un modèle de **diffusion régulière** simple, s'apparentant à un protocole de "gossip" ou "epidemic routing". À chaque intervalle, un routeur envoie **tous les enregistrements** qu’il possède dans sa base de données d'état de liens locale (`local_lsdb`) à **tous ses voisins directs**.

  

Cependant, il existe une subtilité pour optimiser la propagation des changements :

  

* Si un routeur reçoit une information concernant l'un de **ses propres sous-réseaux**, il met immédiatement à jour le timestamp de cet enregistrement avec l'heure actuelle avant de le renvoyer. Cela garantit que les informations concernant le routeur lui-même sont toujours perçues comme les plus fraîches et se propagent rapidement.

  

### Traitement des messages (`on_receive`)

  

La fonction `on_receive` est la partie la plus importante pour la réception des informations de routage. Elle est appelée chaque fois qu'un paquet UDP est reçu sur le port d'écoute du routeur (ici, le port 8080).

  

1.  **Réception du message** : Le routeur écoute sur une socket UDP et reçoit le datagramme entrant.

  

2.  **Désérialisation** : Le contenu du paquet est désérialisé en une structure `RouterDeclaration`. Si la désérialisation échoue (par exemple, si le message est mal formé ou corrompu), l'exception est capturée et le message est simplement ignoré, garantissant la robustesse du système sans le faire planter.

  

3.  **Mise à jour de la LSDB** : La déclaration reçue est passée à la fonction `add_router_declaration`. Cette fonction est responsable de l'insertion de la nouvelle information dans la `local_lsdb` du routeur, en appliquant les règles de fraîcheur basées sur le timestamp : une déclaration plus récente remplace une ancienne.

  

4.  **Calcul et Application des Routes** : Une fois la `local_lsdb` mise à jour, le routeur déclenche un nouveau calcul des chemins les plus courts (`compute_all_routes` utilisant Dijkstra) à partir de sa perspective. Il compare ensuite ces nouvelles routes aux routes déjà installées dans le système (`current_system_routes`) et effectue les opérations nécessaires :

  

*  **Nettoyage** : Les routes obsolètes (qui ne sont plus dans les chemins optimaux calculés) sont supprimées du système via la fonction `delete_route`.

  

*  **Ajout** : Les nouvelles routes ou les routes mises à jour sont ajoutées ou remplacées dans la table de routage du système via la fonction `add_route`.

  

Ce processus assure que la table de routage du système d'exploitation reflète toujours la meilleure topologie connue par le protocole.

  

### Mises à jour périodiques (`on_update`)

  

La fonction `on_update` est déclenchée périodiquement (toutes les 5 secondes avec l'appel `select` et avec un timeout). Elle assure la régularité des opérations de maintenance et de propagation de l'état de lien.

  

1.  **Mise à jour des déclarations locales** : Le routeur met à jour le timestamp de ses propres déclarations dans la `local_lsdb` à l'heure actuelle. Cela garantit que ses propres informations sont toujours "fraîches" et continuent d'être propagées.

  

2.  **Envoi des déclarations** : Le routeur envoie ensuite **toutes** les déclarations présentes dans sa `local_lsdb` à tous ses voisins connus. Ce mécanisme de "gossip" assure la diffusion des informations d'état de lien à travers le réseau.

  

3.  **Recalcul et Application des Routes** :Comme `on_receive`, `on_update` recalcule également les chemins optimaux et met à jour la table de routage du système. Cela permet de rattraper d'éventuels états non cohérents ou des routes qui n'auraient pas été mises à jour immédiatement après une réception.

  

* Les routes obsolètes sont nettoyées (`cleanup_stale_system_routes`).

  

* Les nouvelles routes sont ajoutées ou mises à jour (`add_route`).

  

L'interaction entre `on_receive` et `on_update` est très importante : `on_receive` gère les événements asynchrones (arrivée de paquets), tandis que `on_update` assure la propagation régulière et la cohérence de l'état du réseau.

  

### Différences et Optimisations par rapport à d'autres protocoles

  

Notre protocole se distingue des protocoles de routage standard comme OSPF par plusieurs choix qui en font une solution unique et ciblée :

  

*  **Simplicité Extrême** : Contrairement à OSPF, qui utilise de multiples types de LSA (Link-State Advertisements) pour différentes informations (routeurs, réseaux, inter-zones, externes, etc.), notre protocole n'utilise qu'un **seul type de message de déclaration**. Cette simplification drastique réduit la complexité de l'implémentation et de la maintenance.

  

*  **Absence d'états d'adjacence complexes** : Il n'y a pas d'états `Exstart`, `Exchange`, `Loading`, etc., comme dans OSPF. La "découverte" de voisin est implicite par la réception de messages.

  

*  **Gestion de la fraîcheur par Timestamp** : Plutôt que des champs spécifiques de `LSA Age` avec des `MaxAge` et des `refresh rate` et des inondations explicites pour les suppressions, notre protocole utilise un simple timestamp absolu et une règle de suppression basée sur le vieillissement passif. Cela élimine la nécessité de messages de "flush" explicites ou de mises à jour périodiques lourdes.

  

*  **Philosophie "Least Possible Sent"** : Bien que le protocole envoie "tous les enregistrements" à chaque fois, cela est considéré comme une optimisation dans le contexte de sa simplicité. L'absence de mécanismes complexes de détection de changements incrémentaux, d'élections de DR/BDR, ou de synchronisation d'état complets simplifie la logique qui rendent le protocole plus léger en termes de code et de surcharge de traitement pour le routeur. Pour des réseaux de petite à moyenne taille, cette approche peut être suffisante et plus facile à gérer. Cela dit, pour les très grands réseaux, cela deviendrait effectivement un point faible en termes de bande passante.

  

*  **Rôle unique de R0** : L'intégration d'un routeur `R0` comme "default originate" est une fonctionnalité simple pour assurer une connectivité par défaut sans nécessiter de configurations ou de LSA spécifiques pour les routes par défaut.

  

Ainsi, notre protocole est une implémentation simplifiée et robuste d'un algorithme à état de liens, il a été fait pour la facilité de développement et la résilience, plutôt que pour l'optimisation maximale de la bande passante dans les très grands réseaux. 
  

## Fonctionnalités

  

* Possibilité de choisir sur **quelles interfaces** le protocole est actif. Cela permet une granularité dans l'application du routage dynamique, évitant le traitement des interfaces non pertinentes.

  

*  **Détection de la défaillance d'un lien** (Tolérance aux pannes) : Grâce au mécanisme de timestamp et d'expiration, le protocole peut implicitement détecter quand un lien ou un routeur ne s'annonce plus, et supprimer l'information obsolète de la topologie.

  

*  **Mise à jour de la base interne** qui contient la topologie du réseau (`local_lsdb`) : Cette base est dynamiquement mise à jour avec les informations reçues et auto-nettoyée.

  

*  **Calcul du chemin optimal** pour atteindre un réseau : Le protocole utilise une implémentation de l'algorithme de **Dijkstra** pour déterminer le chemin de moindre coût vers n'importe quel sous-réseau connu.

  

*  **Possibilité de lister les routeurs voisins** : Une fonctionnalité de débogage et de surveillance est incluse pour afficher les routeurs directement connectés ou avec des sous-réseaux en commun.

  

### Fonctionnalités non encore implémentées

  

* Possibilité de choisir un routeur comme routeur par défaut (ceci est prévu mais n'est pas encore entièrement fonctionnel).

  

* La réception de paquets modifiant la topologie du réseau fera temporairement diminuer l'intervalle d'envoi des paquets. Cela permettrait une convergence plus rapide lors de changements majeurs dans le réseau.

  

## Points forts et faibles

  

### Fiabilité

  

Le protocole repose sur **UDP**, ce qui implique qu'il n’y a **aucune garantie de réception** des paquets intègres ou de leur ordre. Cependant, notre code est capable, dans une certaine mesure, de **détecter les paquets mal formés**, qui sont alors ignorés, ce qui améliore la robustesse face à des données corrompues.

  

De plus, la stratégie de **renvoi multiple des mêmes informations** augmente la probabilité que les messages finissent par atteindre leur destination, même en cas de pertes. La probabilité que **six paquets successifs soient perdus** est extrêmement faible, ce qui rend le protocole *suffisamment fiable* pour sa tâche malgré l'absence de retransmissions au niveau transport.

  

### Sécurité

  

**Aucun mécanisme de sécurité** n’a été mis en place dans cette version. Les messages sont envoyés **en clair sur le réseau**, ce qui constitue une vulnérabilité majeure. Un attaquant connaissant le format du protocole pourrait :

  

*  **Forger ses propres trames** : Il pourrait injecter de fausses informations, faisant croire aux routeurs "fiables" des topologies erronées.

  

*  **Rendre le réseau indisponible (Déni de Service)** : En modifiant les tables de routage pour les rendre invalides, redirigeant le trafic vers des destinations inexistantes, ou en **saturant les routeurs** avec un grand nombre d'enregistrements fictifs et non valides, ce qui consommerait des ressources CPU et mémoire, ralentissant considérablement le réseau.

  

C'est un domaine où des améliorations futures seront faites, notamment l'ajout de mécanismes d'authentification et de chiffrement.

  

### Robustesse

  

Le code a été conçu selon le principe de **zero trust** : chaque fonction valide ses entrées et gère les cas inattendus, ce qui permet d’avoir des fonctions qui ne lancent aucune **exception non contrôlée**. Cela garantit que le programme ne crash pas face à des données ou des états internes qui ne valent rien.

  

Grâce à des tests unitaires exhaustifs (décrits plus en détail plus loin), nous avons pu valider que le code se comporte correctement, même en présence de cas inattendus. Ces tests garantissent que les fonctions fonctionnent aussi bien dans les cas **normaux** que dans les cas **erronés**.

  

> Une anecdote illustre bien la robustesse du code :

> Lors de la création des tests unitaires, plusieurs bases d’états des liens ont été créées. À un moment, une **inattention dans le nommage des variables** a provoqué une incohérence : certains routeurs simulés étaient reliés à des réseaux qu’ils n’étaient pas censés connaître.

> Malgré cela, **le protocole n’a provoqué aucun crash**. Il s’est simplement mis à essayer d’envoyer des paquets vers des réseaux inexistants dans la configuration actuelle, ce qui montre sa capacité à gérer les erreurs de manière contrôlée, sans provoquer d'interruption inattendue du service.

  

## Résultats des tests

  

Pour tester notre protocole, des **tests unitaires** ont été créés afin de garantir un fonctionnement correct et robuste.

  

Dans le répertoire `logic`, le fichier `unit_test.cpp` contient l'ensemble des tests qui assurent un fonctionnement dans les cas extrêmes et valident le comportement attendu.

  

Les tests unitaires sont lancés avec les commandes suivantes :

  
  

```bash

  

./compile  ./unit_test

  

```

  

Le résultat attendu à l'exécution de `./unit_test` est `[Tous les tests]`, indiquant que toutes les vérifications ont réussi.

  

Ces tests unitaires vérifient les fonctionnalités en faisant abstraction du transfert réseau réel. Une démonstration séparée, montrant les échanges réseau et la convergence en temps réel, a été réalisée pour valider le comportement global.

  

### Résumé des tests

  

*  **Validation du nom du routeur** : Vérifie la conformité des noms d'hôtes (`RXXXXX`).

  

*  **Validation des CIDR reçus** : Assure que les adresses IP avec masque de sous-réseau sont dans un format valide.

  

*  **Test de sérialisation et dé-sérialisation des messages** : Confirme que les objets `RouterDeclaration` peuvent être correctement convertis en chaînes de caractères pour la transmission et reconstitués fidèlement à la réception.

  

*  **Test d'affichage de la base locale** en injectant directement les données dans la structure : Vérifie la capacité du routeur à maintenir et afficher sa propre vue de la topologie.

  

*  **Test d'ajout depuis la fonction côté serveur** : Valide la logique d'intégration des nouvelles déclarations reçues.

  

*  **Test d'ajout d'un même enregistrement avec un coût différent** : S'assure que les mises à jour des coûts de lien sont gérées correctement.

  

*  **Test de mise à jour du timestamp** : Confirme que le timestamp est actualisé pour les informations du routeur local et lors de la réception d'informations plus récentes.

  

*  **Test de l'auto-suppression des enregistrements trop anciens** : Valide le mécanisme de vieillissement et de nettoyage des données obsolètes.

  

*  **Test si trois routeurs peuvent converger** (les messages ne sont pas envoyés, seules les structures sont échangées dans le code) : Test fondamental de l'algorithme d'état de liens, vérifiant que les routeurs parviennent à une vue cohérente de la topologie.

  

*  **Test de conversion IP du routeur + masque de sous-réseau vers l'IP du sous-réseau** : Valide la fonction de calcul de l'adresse réseau à partir d'une IP et de son masque.

  

*  **Test d'extraction de tous les réseaux existants depuis un routeur** : Vérifie la capacité à identifier tous les sous-réseaux connus.

  

*  **Test d'extraction du nom de tous les routeurs existants depuis un routeur** : Vérifie la capacité à lister tous les routeurs connus dans la topologie.

  

*  **Test de la création d'une matrice depuis la base de données locale** : Valide la construction de la matrice d'adjacence utilisée par l'algorithme de Dijkstra.

  

*  **Test de l'algorithme de sélection du chemin optimal** : Teste la logique de calcul du chemin le plus court (Dijkstra).

  

*  **Test d'affichage de la liste des voisins** : Vérifie la capacité à identifier et présenter les routeurs voisins.

  

## Démonstration et Résultats Visuels

  

Cette partie présente des captures d'écran illustrant le comportement du protocole OSPF développé par Julien et Romain, avant et après la convergence des tables de routage. Cela complète un peu les tests unitaires en montrant l'impact réel du protocole sur des routeurs sous Debian 12, avec ce réseau:

![Table de routage R5 avant protocole](https://github.com/RomainCarlinoIMT/OSPF-IMT/raw/main/pictures/reseau.png)
  

### État Initial du Routeur R5

  

Avant l'activation du protocole, les tables de routage du routeur R5 ne contiennent que les routes directement connectées. Les captures d'écran ci-dessous montrent l'état typique de la table de routage (`ip route show`) et éventuellement des règles de filtrage ou de NAT (`iptables -L`) si elles sont pertinentes à l'environnement de test.

  
 ![Table de routage R5 avant protocole](https://github.com/RomainCarlinoIMT/OSPF-IMT/raw/main/pictures/iprouteavant.png)

  
*Description : Capture d'écran de la commande `ip route` sur le routeur R5, montrant uniquement les routes directes et locales avant le démarrage du protocole OSPF Julien et Romain.*

  

![Table de routage R5 avant protocole](https://github.com/RomainCarlinoIMT/OSPF-IMT/raw/main/pictures/ipa.png)

*Description : Capture d'écran de la commande `ip a` sur le routeur R5

  

### État du Routeur R5 après Convergence


Après le démarrage du protocole et un certain temps (généralement quelques seconds à quelques minutes, en fonction de la taille du réseau et des intervalles de mise à jour), le routeur R5 aura échangé des informations avec ses voisins et convergé. Sa table de routage sera alors remplie avec les routes apprises dynamiquement via notre protocole, reflétant la topologie optimale du réseau.

  ![Table de routage R5 avant protocole](https://github.com/RomainCarlinoIMT/OSPF-IMT/raw/main/pictures/iprouteapres.png)



*Description : Capture d'écran de la commande `ip route` sur le routeur R5, montrant l'ajout des routes indirectes apprises par le protocole OSPF Julien et Romain, avec les next-hops correspondants.*

  
Ces exemples visuels permettent de constater l'efficacité du protocole à construire dynamiquement la table de routage d'un routeur et à maintenir la connectivité à travers le réseau simulé.

  

## Synthèse

  

### Points forts

  

*  **Simplicité** : Un seul type de message à gérer réduit beaucoup la complexité de l’implémentation, facilitant le développement et la maintenance.

  

*  **Diffusion implicite** : Les données circulent sans avoir besoin d’arbres de diffusion complexes ou de chemins spécifiques, simplifiant la logique d'inondation.

  

*  **Actualisation automatique** : Le système se met à jour en continu grâce aux timestamps, assurant la fraîcheur des informations tant que les messages sont reçus.

  

*  **Résilience aux pannes** : Un routeur ou un lien inactif est détecté implicitement grâce à l’expiration des timestamps, sans nécessiter de messages de "down" explicites.

  

### Points faibles

  

*  **Pas d’optimisation de bande passante pour les grands réseaux** : Tous les enregistrements sont envoyés à chaque fois. Et dans un très grand réseau, cela pourrait générer un trafic excessif.

  

*  **Propagation potentiellement lente** en cas de lien faible ou congestionné, en raison de l'absence de mécanismes de retransmission ou de reconnaissance au niveau transport.

  

*  **Pas de protection contre les boucles ou les doublons explicite** : Bien que le timestamp aide, l'absence de mécanismes plus avancés (comme les séquences de LSA dans OSPF) pourrait, dans des scénarios extrêmes, ne pas entièrement prévenir toutes les formes de boucles ou de redondance d'informations si deux routeurs mettent à jour les mêmes enregistrements sans coordination parfaite.

  

*  **Manque de mécanismes de sécurité** : L'absence de vérification d’authentification ou d’intégrité des messages est un problème majeur, rendant le protocole vulnérable aux attaques par injection de données.

  

## Architecture Technique et Implémentation

  

L'implémentation de ce protocole en C++ s'appuie sur plusieurs bibliothèques standards, choisies pour leur efficacité, leur robustesse et leur intégration native au langage, minimisant ainsi les dépendances externes :

  

*  **`<iostream>`** : Pour les opérations d'entrée/sortie de base (affichage de débogage, messages d'état). Indispensable pour toute interaction console.

  

*  **`<string>`** : Pour la manipulation des chaînes de caractères (noms de routeurs, adresses IP, sérialisation/désérialisation des messages). C'est la base pour le traitement des données textuelles.

  

*  **`<vector>`** : Utilisé pour les collections dynamiques, notamment pour stocker les segments lors de la désérialisation, ou les listes de nœuds dans les algorithmes de graphe.

  

*  **`<map>`** : Essentiel pour la structure `local_lsdb` (Link-State Database). Un `std::map<std::string, std::map<std::string, RouterDeclaration>>` permet de stocker les déclarations : la première clé est le nom du routeur, et la seconde est l'IP/masque du lien pour ce routeur. Cette structure est intuitive pour représenter une base de données d'état de liens.

  

*  **`<set>`** : Utilisé pour garantir l'unicité des éléments, par exemple lors de l'extraction de tous les sous-réseaux ou noms de routeurs uniques (`get_all_subnets`, `get_all_routers`).

  

*  **`<algorithm>`** : Fournit des fonctions utilitaires comme `std::all_of` (pour valider que des caractères sont des chiffres) ou `std::find` (pour rechercher des éléments dans des conteneurs), simplifiant le code et améliorant sa lisibilité.

  

*  **`<chrono>`** : Crucial pour la gestion des timestamps. `std::chrono::system_clock::now().time_since_epoch().count()` permet d'obtenir l'horodatage précis en millisecondes, essentiel pour le mécanisme de fraîcheur des informations.

  

*  **`<sstream>`** : Utilisé pour la sérialisation et désérialisation des chaînes de caractères (`std::stringstream`), transformant facilement des objets en chaînes et vice-versa.

  

*  **`<queue>`** : Spécifiquement `std::priority_queue`, est fondamentale pour l'implémentation de l'algorithme de Dijkstra, permettant de sélectionner efficacement le nœud de plus faible coût à chaque étape.

  

*  **`<limits>`** : Utilisé pour obtenir la valeur maximale d'un type entier (`std::numeric_limits<int>::max()`), nécessaire pour représenter l'infini dans l'algorithme de Dijkstra (pour les coûts de liens non connectés).

  

*  **`<stdexcept>`** : Permet de gérer les erreurs de manière structurée via des exceptions (`std::invalid_argument`, `std::out_of_range`, `std::runtime_error`), assurant la robustesse du code face à des données d'entrée invalides ou des problèmes internes.

  

*  **`<bits/stdc++.h>`** : Bien que pratique pour la compilation rapide, cette inclusion est souvent évitée dans les projets de production car elle inclut toutes les bibliothèques standards, ce qui peut augmenter le temps de compilation et la taille du binaire. Pour un projet de cette envergure, les inclusions spécifiques sont préférables, comme listé ci-dessus.

  

*  **Bibliothèques Netlink (Linux)** : L'implémentation utilise intensément les bibliothèques Netlink (`<netlink/netlink.h>`, `<netlink/socket.h>`, `<netlink/route/route.h>`, etc.) pour interagir directement avec le noyau Linux afin de gérer la table de routage du système.

  

*  **`add_route`** : Ajoute une route statique au système. Elle inclut une logique pour supprimer d'abord toute route existante pour la même destination afin de garantir que seule la route optimale calculée est présente. Elle tente également de trouver l'interface de sortie appropriée (`ifindex`) en fonction de la passerelle.

  

*  **`delete_route`** : Supprime une route spécifique de la table de routage du système.

  

*  **`delete_indirect_routes`** : Une fonction essentielle au démarrage et lors du nettoyage, qui parcourt la table de routage du système pour supprimer toutes les routes qui ont une passerelle (routes indirectes), afin de s'assurer que notre protocole est le seul à gérer ces entrées et éviter les conflits avec d'autres sources de routage ou les routes résiduelles d'exécutions précédentes. Ces fonctions sont cruciales pour que le protocole "prend le contrôle" effectif du routage sur la machine.

  

### Considérations sur la Mémoire

  

La gestion de la mémoire est principalement influencée par la taille de la `local_lsdb`. Chaque `RouterDeclaration` stockée dans cette carte représente une information sur un lien spécifique dans le réseau.

  

*  **Croissance de la `local_lsdb`** : La `local_lsdb` grandit avec le nombre de routeurs et de liens dans le réseau. Chaque entrée stocke le nom du routeur (chaîne), l'IP/masque (chaîne), un coût (entier) et un timestamp (long long). Pour un réseau de taille normale, l'impact mémoire est faible. Cependant, dans de très grands réseaux avec de nombreux routeurs et liens, la quantité de données stockées pourrait devenir importante.

  

*  **Mécanisme de Nettoyage** : La fonction `cleanup_old_declarations` est vraiment importante pour la gestion de la mémoire. En supprimant les déclarations obsolètes (celles dont le timestamp est trop ancien), elle empêche la `local_lsdb` de croître indéfiniment et de consommer des ressources excessives, ce qui est vital pour la stabilité à long terme du routeur. Elle garantit que seules les informations actives et pertinentes sont conservées en mémoire.

  
### Fichier de Configuration

Le routeur nécessite un **fichier de configuration initial**, rempli manuellement. Ce fichier sert uniquement à spécifier les **interfaces réseau** locales qui participeront au protocole.

Chaque ligne du fichier contient une **adresse IP et un masque de sous-réseau**, correspondant à un réseau que le routeur annonce et utilise pour communiquer avec ses voisins.

**Exemples de lignes valides dans le fichier :**
192.168.2.0/24
10.2.0.0/24

> ⚠️ Seules les interfaces listées ici seront activées dans le protocole. Toute autre interface locale sera ignorée.

Bien que cette configuration soit manuelle, **une fois le routeur lancé, la découverte et la gestion de la topologie se font automatiquement**, sans intervention humaine.

Cela en fait un système à la fois **simple** et **"plug-and-play"**, une fois les interfaces initialement définies.
  

### Ports de Communication

  

Comme mentionné précédemment, le protocole utilise des ports UDP pour ses communications. Un port spécifique est configuré pour l'envoi et la réception des messages de déclaration. Le choix d'UDP, sans les garanties de TCP, est compensé par la nature périodique et la redondance des envois d'informations, ce qui est une approche courante pour les protocoles de routage en temps réel où la rapidité est privilégiée sur la fiabilité transactionnelle.

```