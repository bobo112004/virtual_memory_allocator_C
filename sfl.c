//Popescu Bogdan Stefan 312CAa 2023-2024
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <math.h>

#define DIE(assertion, call_description)                \
	do {                                \
		if (assertion) {                    \
			fprintf(stderr, "(%s, %d): ",           \
					__FILE__, __LINE__);        \
			perror(call_description);           \
			exit(errno);                        \
		}                           \
	} while (0)

//structura pentru info. aditionale la un nod, stocat in structura generica
typedef struct info {
	char *str;
	unsigned int address;
	unsigned int node_size;
} info;

//structura generica a nodului
typedef struct dll_node_t {
	void *data;
	struct dll_node_t *prev, *next;
} dll_node_t;

//structura listei dublu inlantuite (necirculara)
typedef struct doubly_linked_list_t {
	dll_node_t *head;
	unsigned int size;
	unsigned int data_size;
} doubly_linked_list_t;

//structura pentru vectorul de liste + informatii adidtionale
typedef struct segregated_free_list {
	doubly_linked_list_t **list;
	unsigned int list_nr;
	unsigned int rec;
} segregated_free_list;

//structura pentru a monitoriza informatiile necesare la dump_memory
typedef struct monitorizare {
		unsigned int dimensiune_heap;
		unsigned int malloc, frag, free;
		unsigned int mem, free_blocks, alloc_blocks;
} monitorizare;

//functie care creeaza o lista dublu inlantuita si intoarce pointer la aceasta
doubly_linked_list_t *dll_create(unsigned int data_size)
{
	doubly_linked_list_t *list = calloc(1, sizeof(doubly_linked_list_t));
	DIE(!list, "Nu s-a putut aloca memorie pentru o lista!\n");
	list->data_size = data_size;
	return list;
}

unsigned int min(unsigned int a, unsigned int b)
{
	return a < b ? a : b;
}

//functie care adauga un nod la o lista (indiferent de caz)
void dll_add_nth_node(doubly_linked_list_t *list, unsigned int n,
					  void *new_data, unsigned int data_size)
{
	dll_node_t *new, *current = list->head;
	new = calloc(1, sizeof(dll_node_t));
	DIE(!new, "Nu s-a putut aloca memorie pentru un nou nod!\n");
	if (n > list->size)
		n = list->size - 1;
	if (list->size == 0) {
		list->head = new;
	} else if (n == 0) {
		list->head = new;
		new->next = current;
		current->prev = new;
	} else {
		for (unsigned int i = 0; i < n - 1; i++)
			current = current->next;
		new->next = current->next;
		new->prev = current;
		if (current->next)
			current->next->prev = new;
		current->next = new;
	}
	//se aloca memorie pentru ce va fi stocat in void *data
	new->data = calloc(1, data_size);
	DIE(!new->data, "Nu s-a putut aloca memorie in nod pentru data!\n");
	//se copiaza continutul de la new_data, astfel se poate modifica
	//si in afara listei (daca este necesar)
	memcpy(new->data, new_data, data_size);
	list->size++;
}

//functia sterge un nod si intoarce pointer catre nodul sters
dll_node_t *dll_remove_nth_node(doubly_linked_list_t *list, unsigned int n)
{
	dll_node_t *current = list->head;
	if (list->size == 1) {
		list->size--;
		list->head = NULL;
		return current;
	}
	if (n >= list->size)
		n = list->size - 1;
	while (n--)
		current = current->next;
	if (!current->prev) {
		list->head = current->next;
		current->next->prev = NULL;
	} else if (!current->next) {
		current->prev->next = NULL;
	} else {
		current->prev->next = current->next;
		current->next->prev = current->prev;
	}
	list->size--;
	return current;
}

//functie folosita pentru eliberarea unei intregi liste (absolut tot)
void dll_free(doubly_linked_list_t **pp_list)
{
	dll_node_t *current = (*pp_list)->head;
	for (int i = 0; i < (int)(*pp_list)->size - 1; i++) {
		current = current->next;
		free(((info *)current->prev->data)->str);
		free((info *)current->prev->data);
		free(current->prev);
	}
	if (current) {
		free(((info *)current->data)->str);
		free(current->data);
		free(current);
	}
	free((*pp_list));
}

//functie care gaseste pozitia la care un nod trebuie adaugat intr-o lista
//astfel incat aceasta sa ramana ordonata crescator in functie de adresse
unsigned int find_position(doubly_linked_list_t *list, unsigned int address)
{
	if (list->size == 0)
		return 0;
	dll_node_t *current = list->head;
	unsigned int next = ((info *)current->data)->address, i = 0;
	//parcurgem pana cand am gasit o adresa mai mare si intoarcem pozitia
	while (address > next) {
		i++;
		current = current->next;
		if (!current)
			break;
		next = ((info *)current->data)->address;
	}
	return i;
}

//functia care se ocupa de comanda DESTROY_HEAP
void free_heap(segregated_free_list **seg_list, doubly_linked_list_t **memory,
			   monitorizare **stats)
{
	//parcurgem vectorul de liste si stergem fiecare lista (complet)
	for (unsigned int i = 0; i < (*seg_list)->list_nr; i++)
		dll_free(&((*seg_list)->list[i]));
	//stergem apoi structurile si lista dublu inlantuita pentru memoria alocata
	free((*seg_list)->list);
	free(*seg_list);
	dll_free(memory);
	free(*stats);
}

//functia care se ocupa de crearea vectorului de liste, intoarce pointer la el
segregated_free_list *init_heap(unsigned int start_address, unsigned int nr,
								unsigned int bytes, unsigned int rec,
								monitorizare *stats)
{
	segregated_free_list *seg_list = calloc(1, sizeof(segregated_free_list));
	seg_list->list = calloc(1, nr * sizeof(doubly_linked_list_t *));
	seg_list->list_nr = nr;
	seg_list->rec = rec;
	int l = 8;
	info info1;
	doubly_linked_list_t *bucket;
	//pentru listele initiale tinem cont de dimensiunea acestora (8,16,32...)
	//si adaugam noduri la liste astfel incat sa avem la fiecare bytes octeti
	for (unsigned int i = 0; i < nr; i++) {
		seg_list->list[i] = dll_create(l);
		bucket = seg_list->list[i];
		for (unsigned int j = 0; j < bytes / l; j++) {
			//vom asocia lui void *data si o structura pt operatiile viitoare
			info1.address = start_address;
			info1.node_size = l;
			info1.str = calloc(1, l + 1);
			DIE(!info1.str, "Nu s-a putut aloca memorie pentru WRITE!");
			dll_add_nth_node(bucket, bucket->size, &info1,
							 sizeof(struct info));
			stats->free_blocks++;
			start_address = start_address + l;
		}
		l *= 2;
	}
	stats->dimensiune_heap = bytes * nr;
	return seg_list;
}

//functie care verifica daca exista sau nu o lista cu blocuri de dimensiune
//data_size in seg free list, iar in cazul in care nu exista, o creeaza
doubly_linked_list_t *get_list(segregated_free_list *seg_list,
							   unsigned int nr_bytes)
{
	//verificam mai intai daca lista cu dimensiunea cautata exista deja
	for (unsigned int i = 0; i < (seg_list)->list_nr; i++) {
		if ((seg_list)->list[i]->data_size == nr_bytes)
			return (seg_list)->list[i];
		if ((seg_list)->list[i]->data_size > nr_bytes)
			break;
	}
	//suntem in situatia in care nu exista lista, realocam vectorul cu + 1 si
	//apoi introducem lista la locul ei astfel incat sa fie sortate in ordinea
	//crescatoare a dimensiunilor blocurilor
	(seg_list)->list_nr++;
	seg_list->list = realloc(seg_list->list, seg_list->list_nr *
							 sizeof(doubly_linked_list_t *));
	DIE(!seg_list->list, "Memoria nu se poate realoca, e insufucienta!\n");
	unsigned int i = 0;
	for (; i < (seg_list)->list_nr - 1; i++) {
		if ((seg_list)->list[i]->data_size > nr_bytes)
			break;
	}
	//pe pozitia i in vectorul de liste trebuie sa introducem lista noastra,
	//vom shifta totul
	for (unsigned int j = (seg_list)->list_nr - 1; j > i; j--)
		(seg_list)->list[j] = (seg_list)->list[j - 1];
	(seg_list)->list[i] = dll_create(nr_bytes);
	return (seg_list)->list[i];
}

//functia care se ocupa de comanda MALLOC
void seg_list_malloc(segregated_free_list *seg_list,
					 doubly_linked_list_t *memory, unsigned int nr_bytes,
					 monitorizare *stats)
{
	unsigned int i = 0;
	dll_node_t *current;
	//cautam o lista cu un bloc mai mare sau egal si verificam daca exista
	for (; i < seg_list->list_nr; i++) {
		if (seg_list->list[i]->data_size >= nr_bytes && seg_list->list[i]->size)
			break;
	}
	if (i >= seg_list->list_nr) {
		printf("Out of memory\n");
		return;
	}
	stats->malloc++;
	//scoatem blocul si dupa decidem ce facem cu el
	current = dll_remove_nth_node(seg_list->list[i], 0);
	unsigned int address = ((info *)current->data)->address;
	unsigned int poz = find_position(memory, address);
	if (seg_list->list[i]->data_size == nr_bytes) {
		//il mutam cu totul in lista de memorie alocata
		dll_add_nth_node(memory, poz, (info *)current->data, sizeof(info));
		free(current->data);
		free(current);
		stats->free_blocks--;
		stats->alloc_blocks++;
	} else {
		//il fragmentam, bucata alocata o facem intr-un alt nod pe care il
		//adaugam in lista de memorie alocata
		doubly_linked_list_t *bucket;
		stats->frag++;
		info info1;
		info1.address = address + nr_bytes;
		info1.node_size = ((struct info *)current->data)->node_size - nr_bytes;
		info1.str = calloc(1, info1.node_size + 1);
		DIE(!info1.str, "Nu s-a putut aloca memorie pentru WRITE!\n");
		((struct info *)current->data)->node_size = nr_bytes;
		dll_add_nth_node(memory, poz, (struct info *)current->data,
						 sizeof(struct info));
		//gasim unde sa adaugam ce ramane din nod in vectorul de liste
		bucket = get_list(seg_list, info1.node_size);
		poz = find_position(bucket, info1.address);
		dll_add_nth_node(bucket, poz, &info1, sizeof(struct info));
		free(current->data);
		free(current);
		stats->alloc_blocks++;
	}
	stats->mem += nr_bytes;
}

//functia care se ocupa de functia FREE
void free_memory(segregated_free_list *seg_list, doubly_linked_list_t *memory,
				 unsigned int address, monitorizare *stats)
{
	dll_node_t *current;
	doubly_linked_list_t *bucket;
	current = memory->head;
	if (!address)
		return;
	//cautam adresa care trebuie sa fie si inceput de bloc
	for (unsigned int i = 0; i < memory->size; i++) {
		if (((info *)current->data)->address == address) {
			stats->free++;
			//scoatem nodul din lista de memorie alocata si il mutam in
			//vectorul de liste dupa acelasi principiu ca la MALLOC
			current = dll_remove_nth_node(memory, i);
			bucket = get_list(seg_list, ((info *)current->data)->node_size);
			unsigned int poz = find_position(bucket, address);
			dll_add_nth_node(bucket, poz, (info *)current->data, sizeof(info));
			stats->mem -= ((info *)current->data)->node_size;
			free(current->data);
			free(current);
			stats->alloc_blocks--;
			stats->free_blocks++;
			return;
		}
		current = current->next;
	}
	//in cazul in care nu am gasit adresa sau nu este inceput de bloc
	printf("Invalid free\n");
}

//functie care gaseste si intoarce pointer la un nod/bloc care contine o
//anumita adresa (poate fi si in "interior")
dll_node_t *search_address(doubly_linked_list_t *memory, unsigned int address)
{
	dll_node_t *current = memory->head;
	unsigned int c_address, node_size;
	//parcurgem lista si cand gasit un nod care contine acea adresa, ne oprim
	for (unsigned int i = 0; i < memory->size; i++) {
		c_address = ((info *)current->data)->address;
		node_size = ((info *)current->data)->node_size;
		//conditia necesara este ca adresa sa se afle intre adresa de inceput
		//si adresa ultimului octet (adresa_start + size - 1)
		if (address >= c_address && address < c_address + node_size)
			return current;
		//lista fiind sortata crescator dupa adrese, nu are rost sa mai cautam
		//daca adresa noastra e mai mare decat adresa de la un anumit nod
		if (address < c_address)
			break;
		current = current->next;
	}
	return NULL;
}

//functie care verifica daca la WRITE sau READ se apeleaza o zona nealocata
unsigned int check_seg_fault(doubly_linked_list_t *memory,
							 unsigned int address, unsigned int nr_bytes)
{
	dll_node_t *current;
	unsigned int idx1, idx2;
	current = search_address(memory, address);
	while (current && ((info *)current->data)->address == address) {
		//daca inca mai trebuie accesati octeti, ne mutam la urmatorul nod
		//si verificaam daca e continuarea nodului din trecut dpdv. al adresei
		idx1 = address - ((info *)current->data)->address;
		idx2 = min(idx1 + nr_bytes, ((info *)current->data)->node_size) - 1;
		nr_bytes -= idx2 - idx1 + 1;
		if (!nr_bytes)
			return 0;
		address = address + idx2 - idx1 + 1;
		current = current->next;
	}
	return 1;
}

//functia DUMP_MEMORY care se foloseste de cele trei structuri principale
void memory_dump(monitorizare *stats, segregated_free_list *seg_list,
				 doubly_linked_list_t *memory)
{
	printf("+++++DUMP+++++\n");
	printf("Total memory: %u bytes\n", stats->dimensiune_heap);
	printf("Total allocated memory: %u bytes\n", stats->mem);
	printf("Total free memory: %u bytes\n",
		   stats->dimensiune_heap - stats->mem);
	printf("Free blocks: %u\n", stats->free_blocks);
	printf("Number of allocated blocks: %u\n", stats->alloc_blocks);
	printf("Number of malloc calls: %u\n", stats->malloc);
	printf("Number of fragmentations: %u\n", stats->frag);
	printf("Number of free calls: %u\n", stats->free);
	doubly_linked_list_t *bucket;
	dll_node_t *current;
	//parcurgem listele si afisam ce exista
	for (unsigned int i = 0; i < seg_list->list_nr; i++) {
		bucket = seg_list->list[i];
		if (bucket->size) {
			current = bucket->head;
			printf("Blocks with %u bytes - %u free block(s) :",
				   bucket->data_size, bucket->size);
			while (current) {
				printf(" 0x%x", ((info *)current->data)->address);
				current = current->next;
			}
			printf("\n");
		}
	}
	printf("Allocated blocks :");
	current = memory->head;
	unsigned int address, node_size;
	//parcurgem si lista de memorie alocata
	for (unsigned int i = 0; i < memory->size; i++) {
		address = ((info *)current->data)->address;
		node_size = ((info *)current->data)->node_size;
		printf(" (0x%x - %u)", address, node_size);
		current = current->next;
	}
	printf("\n");
	printf("-----DUMP-----\n");
}

//functia WRITE
void write_memory(doubly_linked_list_t *memory, char *string,
				  unsigned int nr_bytes, unsigned int address,
				  segregated_free_list *seg_list, monitorizare *stats)
{
	dll_node_t *current = search_address(memory, address);
	char *temp;
	unsigned int idx1, idx2;
	nr_bytes = min(nr_bytes, strlen(string));
	//verificam mai intai daca suntem in cazul de Segmentation fault
	if (check_seg_fault(memory, address, nr_bytes)) {
		printf("Segmentation fault (core dumped)\n");
		memory_dump(stats, seg_list, memory);
		free_heap(&seg_list, &memory, &stats);
		exit(0);
	}
	//avem deja garantat ca toate adresele sunt alocate/valide, trebuie doar
	//sa parcurgem si sa scriem la adresele anume
	while (nr_bytes) {
		//fiecarei adrese ii asociem un index, deci putem afla startul (idx1)
		//si finalul (idx2) unde vom scrie din sirul de caractere
		idx1 = address - ((info *)current->data)->address;
		idx2 = min(idx1 + nr_bytes, ((info *)current->data)->node_size) - 1;
		temp = calloc(1, 600);
		DIE(!temp, "Nu s-a putut aloca memorie pentru un string temporar!\n");
		char *p = ((info *)current->data)->str;
		//scrierea efectiva prin memmove (mai optim pentru locatii suprapuse)
		memmove(temp, p + idx2 + 1, strlen(p + idx2 + 1));
		memmove(p + idx1, string, idx2 - idx1 + 1);
		memmove(string, string + idx2 - idx1 + 1,
				strlen(string + idx2 - idx1 + 1));
		nr_bytes -= idx2 - idx1 + 1;
		address = address + idx2 - idx1 + 1;
		current = current->next;
		free(temp);
	}
}

//functie care se ocupa de READ, cu un rationament asemanator cu WRITE
void read_memory(doubly_linked_list_t *memory, unsigned int nr_bytes,
				 unsigned int address, segregated_free_list *seg_list,
				 monitorizare *stats)
{
	dll_node_t *current = search_address(memory, address);
	unsigned int idx1, idx2;
	//verificarea cazului Segmentation fault
	if (check_seg_fault(memory, address, nr_bytes)) {
		printf("Segmentation fault (core dumped)\n");
		memory_dump(stats, seg_list, memory);
		free_heap(&seg_list, &memory, &stats);
		exit(0);
	}
	//pentru fiecare bloc, aflam indexii intre care citim si ne mutam pe
	//blocul urmator daca mai trebuie citit ceva, avand garantia ca toate
	//memoria este alocata deja
	while (nr_bytes) {
		idx1 = address - ((info *)current->data)->address;
		idx2 = min(idx1 + nr_bytes, ((info *)current->data)->node_size) - 1;
		for (unsigned int i = idx1; i <= idx2; i++)
			printf("%c", (((info *)current->data)->str)[i]);
		nr_bytes -= idx2 - idx1 + 1;
		address = address + idx2 - idx1 + 1;
		current = current->next;
	}
	printf("\n");
}

int main(void)
{
	char command[15], string[601];
	unsigned int start_address, list_nr, bytes, rec, param;
	segregated_free_list *seg_list;
	doubly_linked_list_t *memory;
	monitorizare *stats;
	stats = calloc(1, sizeof(monitorizare));
	DIE(!stats, "Nu s-a putut aloca memorie pentru a monitoriza programul!\n");
	//se vor executa oricate comenzi, pana la intalnirea lui DESTROY_HEAP
	while (1) {
		scanf("%s", command);
		if (!strcmp(command, "INIT_HEAP")) {
			scanf("%x %u %u %u", &start_address, &list_nr, &bytes, &rec);
			seg_list = init_heap(start_address, list_nr, bytes, rec, stats);
			memory = dll_create(0);
		}
		if (!strcmp(command, "MALLOC")) {
			scanf("%u", &param);
			seg_list_malloc(seg_list, memory, param, stats);
		}
		if (!strcmp(command, "FREE")) {
			scanf("%x", &param);
			free_memory(seg_list, memory, param, stats);
		}
		if (!strcmp(command, "WRITE")) {
			scanf("%x", &start_address);
			fgets(string, 600, stdin);
			memmove(string, string + 2, strlen(string + 2));
			//vom modifica string-ul, scapam de ghilimele si extragem
			//numarul de octeti trimis dupa string
			char *p = strtok(string, "\"");
			p = strtok(NULL, "\"");
			param = atoi(p);
			string[p - string - 1] = '\0';
			write_memory(memory, string, param, start_address, seg_list, stats);
		}
		if (!strcmp(command, "READ")) {
			scanf("%x%d", &start_address, &bytes);
			read_memory(memory, bytes, start_address, seg_list, stats);
		}
		if (!strcmp(command, "DUMP_MEMORY"))
			memory_dump(stats, seg_list, memory);
		if (!strcmp(command, "DESTROY_HEAP")) {
			free_heap(&seg_list, &memory, &stats);
			break;
		}
	}
	return 0;
}
