# OSPF-IMT
Un OSPF à la sauce Julien et Romain

## Description fonctionnelle
### 4.1 Priorités de niveau 1
> - 1.1 Calculer les meilleurs chemins entre chaque routeur et réseaux IP du réseau
local sur la base des chemins les plus courts (nombre de saut), de l’état des liens
(actifs ou inactifs) et de leurs capacités nominales (débits maximaux).
> - 1.2 Mettre à jour le calcul des meilleurs chemins tenant compte des modification
des caractéristiques du réseau (routeurs, liens réseaux).
> - 1.3 Activer ou désactiver le protocole à la demande.
> - 1.4 Pour chaque routeur, spécifier les interfaces réseaux qui doivent être incluses
dans le calcul des meilleurs chemins.
> - 1.5 Modifier la table de routage IPv4 des routeurs en fonction du calcul des
meilleurs chemins.
> - 1.6 Mémoriser la liste des routeurs voisins (adresse IP et nom système des routeurs
voisins) de chaque routeur.
> - 1.7 Afficher à la demande la liste des routeurs voisins d’un routeur.
> - 1.8 Tolérer les pannes du réseau.
 
### 4.2 Priorités de niveau 2
> - 2.1 Minimiser la quantité d’informations échangées entre les routeurs du réseau
> - 2.2 Minimiser la quantité de mémoire nécessaire au bon fonctionnement du
protocole dans les routeurs.
> - 2.3 Minimiser le temps de convergence du protocole.
> - 2.4 Spécifier un routeur comme routeur par défaut pour l’ensemble du réseau
(fonctionnalité « default originate » d’OSPF

### 4.3 Priorités de niveau 3
> - 3.1 Calculer les meilleurs chemins entre chaque routeur et réseaux IP du réseau
local sur la base des chemins les plus courts (nombre de saut), de l’état des liens
(actifs ou inactifs) et de leurs capacités disponibles (débits disponibles).
> - 3.2 Sécuriser les échanges d’information entre les différentes entités.
