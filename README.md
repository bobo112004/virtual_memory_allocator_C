Nume: Popescu Bogdan-Stefan
Grupa: 312CAa

## Tema 1: Segregated Free Lists

### Descriere:

* Codul se foloseste de mai multe functii intermediare pentru a executa comenzile din cerinta, comenzi ce sunt puse
intr-un while ce ruleaza la infinit pana la introducerea comenzii DESTROY_HEAP, sau pana la intalnirea cazului
Segmentation fault in unele functii, fapt care va termina executia programului in acel moment.
* Pentru a retine niste informatii aditionale ce erau esentiale in implementarea mea, dar si pentru a pastra 
genericitatea programului, am ales sa am o structura info care sa retina un sir de caractere (cu atatea caractere cati
octeti are un bloc de memorie), adresa si dimensiunea unui bloc, lucruri ce vor fi puse in void *data cand se adauga
un nod la vector sau la lista de memorie alocata. Nu a fost nevoie sa folosesc si alte date/structuri care puteau fi
puse in void *data.
* Pentru fiecare comanda am preferat sa scriu cate o functie, cu eventuale functii ajutatoare, pentru o modularizare
cat mai buna. De asemenea, m-am folosit si de unele functii facute la laborator, mai exact cele ce lucreaza pe liste
dublu inlantuite (adaugare, stergere, free etc). Pentru INIT_HEAP si DESTROY_HEAP am avut nevoie de aceste functii
in special.
* Pentru MALLOC, am cautat prima lista (care nu este goala) cu noduri/blocuri care au o dimensiune mai mare sau
egala cu parametrul primit de la input, apoi am scos primul nod si, in functie de dimensiunea lui, fie l-am adaugat
in lista de memorie alocata, fie l-am fragmentat iar apoi am adaugat cele doua bucati la listele aferente. Deoarece in
urma fragmentarii pot sa rezulte si alte dimensiuni decat cele initiale, iar apoi va trebui sa caut lista cu exact acea
dimensiune, am ales sa implementez functia get_list, care face exact acest lucru, adaugand o noua lista in vector
atunci cand nu exista lista cautata. Am folosit un principiu asemanator la functia FREE, am gasit nodul apoi l-am scos
si l-am adaugat (folosind get_list) la vectorul de liste.
* In cadrul comenzilor READ si WRITE erau nevoie de mai multe operatii si am ales sa "divizez" implementarea in mai
multe etape/functii. Functia check_seg_fault, dupa cum spune si numele, verifica daca suntem in cazul seg fault, adica
NU toate adresele de care avem nevoie sunt alocate, caz in care intervine procedura seg_fault (folosit atat la WRITE,
cat si la READ). Pentru a gasi nodul care contine o anumita adresa, am ales sa ma folosesc de functia search_address.
Pentru ambele comenzi am ales sa aflu cele doua indexuri intre care voi citi/scrie octeti (caractere ASCII). Astfel,
fiecarei adrese ii "corespunde" un index in string. (Ex: blocul de dimensiune 8 de la 0x0 are un sir cu index 0,1,..7).
Singura diferenta este ca la WRITE a trebuit sa ma folosesc de memmove pentru a copia din string-ul original in bloc,
iar la READ, doar trebuie afisat (printf).

### Comentarii asupra temei:

* Ar exista o implementare mai buna?
* Presupun ca as fi putut sa gasesc o metoda mai optima de cautare a unei liste in vectorul de liste. Totusi, am
adaugat o lista atunci cand nu exista in loc sa creez un numar exagerat de mare de liste de la inceput pentru a
gasi o solutie cat mai optima.
* Ce am reusit sa invat si ce mi-a placut:
* R: Am reusit sa ma obisnuiesc sa lucrez cu listele (in special cele dublu inlantuite) si sa stiu cum sa ma folosesc
de functiile facute la laborator (si ulterior folosite si in tema). Mi-am reamintit de asemenea cum sa lucrez cu
string-uri atunci cand era nevoie sa implementez functia WRITE, dar si atunci cand primeam de la datele de intrare sirul
delimitat de ghilimele si care avea, la sfarsit, un numar ce trebuia sa il iau ca parametru. Am mai invatat faptul ca
unele functii predefinite pot avea alternative mai optime, de exemplu functia str(n)cpy cu alternativa memmove, pe care
am folosit-o atunci cand voiam sa sterg unele caractere, iar sirurile cu care lucram se suprapuneau.
* Ce mi s-a parut mai greu/cea mai grea parte din tema:
* R: Sa imi dau seama de cea mai buna varianta de implementare ca in viitor, pentru alte sub-taskuri ale temei, sa imi
fie usor sa modific sectiuni deja existente fara ca programul sa ruleze incorect. Modularizarea a jucat un rol extrem
de important cand vine vorba de fiecare comanda in parte, deoarece puteam folosi unele functii ajutatoare ca sa ma ocup
de unele implementari "costisitoare", de exemplu Segmentation fault din cadrul WRITE si READ. 