#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <omp.h>

typedef struct _nodo // estructura tipo nodo para árbol o lista de árboles
{
   unsigned char caracter; // caracter al que hace referencia el nodo */
   int frecuencia; //veces que aparece el caracteren el texto
   struct _nodo *sig; //puntero al siguiente nodo de una lista 
   struct _nodo *cero; //Puntero a la rama cero de un árbol 
   struct _nodo *uno; // Puntero a la rama uno de un árbol
} Nodo;

//Estructura para construir una lista para la tabla de codigos
typedef struct _tabla
{
   unsigned char caracter; //Caracter a la que hace referencia el nodo
   unsigned long int bits; // Valor de la codificación del caracter
   unsigned char num_bits; //Número de bits de la codificación
   struct _tabla *sig; //Siguiente elemento de la tabla
} Table;

Table *Tabla;
char nombreDelArchivoDeEntrada[50]={0};
char nombreDelArchivoDeSalida[50]={0};

//prototipos de las funciones
void contarFrecuencia(Nodo** list, unsigned char c);
void ordenarLista(Nodo** list);
void insertarEnOrden(Nodo** header, Nodo *e);
void removerArbol(Nodo *n);
void crearTabla(Nodo *n, int l, int v);
void insertarEnTabla(unsigned char c, int l, int v);
Table *buscarCaracter(Table *Tabla, unsigned char c);
void codificar(FILE* fe, FILE* fs, long int longitud,  int n_threads);

int main(int argc, char *argv[])
{
   printf("****************************************************************************\n");
	printf("                      Proyecto paralelo OpenMP - Codificador\n");
	printf("****************************************************************************\n\n");

   FILE *fe, *fs; //Archivos de entrada y salida
   Nodo *list; //lista de caracteres y frecuencias
   Nodo *arbol; //Arbol de caracteres y frecuencias
   unsigned char c; //auxiliares
   Nodo *p;
   Table *t;
   int num_elementos; //numero de elementos de la tabla
   long int longitud = 0; //tamaño del archivo original
   

   //printf("Ingese el archivo a codificar: ");
   //scanf("%s",nombreDelArchivoDeEntrada);
   //sprintf(nombreDelArchivoDeEntrada,"p.txt");
   sscanf(argv[2], "%s", nombreDelArchivoDeEntrada);

   //printf("\n¿Como desea guardar el archivo? Ej. ArchivoCodificado.txt: ");
   //scanf("%s",nombreDelArchivoDeSalida);
   //sprintf(nombreDelArchivoDeSalida,"salida.txt");
    sscanf(argv[3], "%s", nombreDelArchivoDeSalida);



   // Measure start time
    struct timeval ti;
    gettimeofday(&ti, NULL);
    double start = ti.tv_sec * 1000 + ti.tv_usec / 1000;

   list = NULL;
   fe = fopen(nombreDelArchivoDeEntrada, "r");
   do { //contar las frecuencias
      c = fgetc(fe);
      if( feof(fe) ) {
         break ;
      }
      longitud++; //crece la longitud del archivo
      contarFrecuencia(&list, c);//se actualiza la lista de frecuencias
   } while(1);
   fclose(fe);

   //ordenar la lista de menor a mayor
   ordenarLista(&list);
   arbol = list;//se crea el arbol
   while(arbol && arbol->sig) //mintras haya dos elementos en la lista
   {
      p = (Nodo *)malloc(sizeof(Nodo)); //nuevo arbol
      p->caracter = 0; 
      p->uno = arbol;
      arbol = arbol->sig; //siguiente nodo en la rama cero
      p->cero = arbol;
      arbol = arbol->sig;
      p->frecuencia = p->uno->frecuencia + p->cero->frecuencia; //se suman las frecuencias
      insertarEnOrden(&arbol, p);
   }

   Tabla = NULL;
   crearTabla(arbol, 0, 0);  // Construir la tabla de códigos binarios

   //crear el archivo comprimido
   strcat(nombreDelArchivoDeSalida,".huff");
   fs = fopen(nombreDelArchivoDeSalida, "wb");
   fwrite(&longitud, sizeof(long int), 1, fs);  //escribir la longitud del fichero
   num_elementos = 0; //variable para contar el número de elementos de tabla 
   t = Tabla;
   while(t)
   {
      num_elementos++;
      t = t->sig;
   }

   fwrite(&num_elementos, sizeof(int), 1, fs); //escribir el número de elemenos de tabla
   t = Tabla;
   while(t)
   {
      fwrite(&t->caracter, sizeof(char), 1, fs);
      fwrite(&t->bits, sizeof(unsigned long int), 1, fs);
      fwrite(&t->num_bits, sizeof(char), 1, fs);
      t = t->sig;
   }

   //compresion del archivo
   fe = fopen(nombreDelArchivoDeEntrada, "r");
   //int n_threads=0;
   //#  pragma omp parallel num_threads( n_threads ) 
   codificar(fe, fs, longitud, atoi(argv[1]));
   printf("\nEl archivo se guardó en el directorio actual con la extensión .huff correspondiente al archivo codificado.\n");
   fclose(fe);
   fclose(fs);

   removerArbol(arbol);

   while(Tabla)//borrar la tabla
   {
      t = Tabla;
      Tabla = t->sig;
      free(t);
   }

   // Measure end time
    gettimeofday(&ti, NULL);
    double end = ti.tv_sec * 1000 + ti.tv_usec / 1000;

    // Elapsed time
    printf("%lf milliseconds elapsed\n", (end - start));

   return 0;
}

void codificar(FILE* fe, FILE* fs, long int f_length, int n_threads)
{
   unsigned long int leOrCa; // letra o caracter que se trabaja
   int n_Bits = 0; //Número de bits usados de letra o caracter
   char ch;
   FILE *fas;
   f_length+=1;
   printf("f_length %ld\n", f_length);
   printf("--------------\n");
   char *vectorfile, *finalVectorfile;//[f_length+1];
   char *punteros[n_threads];
   int longitudesDeSalida[n_threads];

   vectorfile = (char*)malloc(sizeof(char)*f_length);
   fas = fopen(nombreDelArchivoDeEntrada, "r");
   rewind(fas);
   fread(vectorfile, f_length, 1, fas);
   vectorfile[f_length] = 0;
   rewind(fas);
   //while((ch=fgetc(fas))!=EOF) {
	//    count++;
	//}
   printf("%ld\n", f_length);

   int my_id = omp_get_thread_num();
   //int n_threads = 1;

   //int local_n = count / n_threads; 

   fclose(fas);
   //printf("%d",count);
   //do {
   #pragma omp parallel private(n_Bits, leOrCa, finalVectorfile) num_threads(n_threads)
   {
   n_Bits = 0;
   leOrCa = 0;
   int index = 0;
   Table *t;
   unsigned char c;
   finalVectorfile = (char*)malloc(sizeof(char)*f_length);
   #  pragma omp for schedule(static) 
      for(int i = 0; i < f_length-1; i++){
         //printf("hilo %d %d/%ld; index %d; n_Bits: %d\n", omp_get_thread_num(), i, f_length, index, n_Bits);
         c = vectorfile[i];
         //printf("hilo %d ************\n", omp_get_thread_num());
         t = buscarCaracter(Tabla, c); //busca c en la tabla
         //printf("hilo %d ////////////////\n", omp_get_thread_num());
         while(n_Bits + t->num_bits > 32) // sacar un Byte si n_Bits + t->num_bits > 32
         {
            c = leOrCa >> (n_Bits-8); //Extrae los 8 bits de mayor peso para escribirlo en el archivo
            //#     pragma omp critical
            //fwrite(&c, sizeof(char), 1, fs);
            finalVectorfile[index++] = c;
            n_Bits -= 8; //Se hace espacio para otro byte
         }
         //printf("hilo %d ************\n", omp_get_thread_num());
         leOrCa <<= t->num_bits; //Se hace espacio para el nuevo caracter
         leOrCa |= t->bits;  //se inserta
         n_Bits += t->num_bits;  //se actualiza el conteo de bits
      //} while(1);
      }
      punteros[omp_get_thread_num()] = finalVectorfile;
      longitudesDeSalida[omp_get_thread_num()] = index;
      //fwrite(finalVectorfile, sizeof(char), index, fs);
      //free(finalVectorfile);
      
   }
   for (size_t i = 0; i < n_threads; i++)
   {
      /* code */
      fwrite(punteros[i], sizeof(char), longitudesDeSalida[i], fs);
      free(punteros[i]);
   }
   
   free(vectorfile);

}

void contarFrecuencia(Nodo** list, unsigned char c) //Actualiza la cuenta de frecuencia del carácter c
{
   Nodo *p, *a, *q;

   if(!*list)  //Si la lista está vacía, el nuevo nodo será lista
   {
      *list = (Nodo *)malloc(sizeof(Nodo)); //nodo nuevo para c en su primer aparición
      (*list)->caracter = c;
      (*list)->frecuencia = 1; 
      (*list)->sig = (*list)->cero = (*list)->uno = NULL;
   }
   else //se busca el caracter en la lista
   {
      p = *list;
      a = NULL;
      while(p && p->caracter < c)
      {
         a = p; //se guarda el elemento actual para insertarlo
         p = p->sig; //se va con el que sigue
      }
      if(p && p->caracter == c) //si la letra se encontó, se aumenta la frecuencia
         p->frecuencia++; 
      else //si no se encontró se inserta un elemento nuevo
      {
         q = (Nodo *)malloc(sizeof(Nodo));
         q->caracter = c;
         q->frecuencia = 1;
         q->cero = q->uno = NULL;
         q->sig = p;  
         if(a) 
            a->sig = q;  
         else 
            *list = q; //si a es NULL el nuevo es el primero 
      }
   }
}

void ordenarLista(Nodo** list) //función que ordena la lista de menor a mayor por frecuencias
{
   Nodo *list2, *a;

   if(!*list) 
      return; //lista vacia
   list2 = *list;
   *list = NULL;
   while(list2)
   {
      a = list2; //toma los elementos para isertarlos en la lista por orden
      list2 = a->sig;
      insertarEnOrden(list, a);
   }
}

void insertarEnOrden(Nodo** header, Nodo *e) //inserta el elemento e en la Lista ordenado por su frecuencia de menor a mayor
{
   Nodo *p, *a;

   if(!*header) //Si header es NULL, es el primer elemento 
   {
      *header = e;
      (*header)->sig = NULL;
   }
   else // si no busca el caracter en la lista ordenada
   {
       
       p = *header;
       a = NULL;
       while(p && p->frecuencia < e->frecuencia)
       {
          a = p; //se guarda el elemento actual para insertar
          p = p->sig; //se avanza al siguiente
       }
       e->sig = p;
       if(a) 
         a->sig = e;
       else //el nuevo será el primero
         *header = e; 
    }
}

/* Recorre el árbol cuya raiz es n y le asigna el código v de l bits */
void crearTabla(Nodo *n, int l, int v) //función recursiva que crea la tabla
{
   if(n->uno)  
      crearTabla(n->uno, l+1, (v<<1)|1);
   if(n->cero) 
      crearTabla(n->cero, l+1, v<<1);
   if(!n->uno && !n->cero) 
      insertarEnTabla(n->caracter, l, v);
}

void insertarEnTabla(unsigned char c, int l, int v) //funcion para insertar elemento en la tabla
{
   Table *t, *p, *a;

   t = (Table *)malloc(sizeof(Table)); 
   t->caracter = c; 
   t->bits = v;
   t->num_bits = l;

   if(!Tabla) //si la tabla esta vacia, el elemento t será el primero
   {
      Tabla = t;
      Tabla->sig = NULL;
   }
   else //si no , lo busca
   {
       p = Tabla;
       a = NULL;
       while(p && p->caracter < t->caracter)
       {
          a = p; //se guarda el elemento actual para insertarlo
          p = p->sig;
       }

       t->sig = p; //se inserta
       if(a) 
         a->sig = t;
       else
          Tabla = t;
    }
}

Table *buscarCaracter(Table *Tabla, unsigned char c) //funcion que busca un caracter en la tabla y devielve un puntero al elemento de la tabla
{
   Table *t;
   t = Tabla;
   while(t && t->caracter != c) 
      t = t->sig;
   return t;
}

void removerArbol(Nodo *n) //función para borrar arbol
{
   if(n->cero) 
      removerArbol(n->cero);
   if(n->uno)  
      removerArbol(n->uno);
   free(n);
}
