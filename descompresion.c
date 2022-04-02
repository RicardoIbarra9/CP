#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

typedef struct _nodo
{
   char num_bits; //el numero de bits de la codificacion
   unsigned char caracter; //caracter al que hace referencia el nodo
   unsigned long int bits; //Valor de la codificación del caracter
   struct _nodo *cero; //puntero a rama cero de un arbol
   struct _nodo *uno; //puntero a la rama 1 de un arbol
} Nodo;

//prototipo
void borrarArbol(Nodo *n);

int main(int argc, char *argv[])
{
   printf("****************************************************************************\n");
	printf("                             Proyecto secuencial - Decodificador\n");
	printf("****************************************************************************\n\n");

   unsigned long int bits; //bits para la decodificación
   char nombreDelArchivoDeEntrada[50]={0}; //nombres de archivos de entrada y salida
   char nombreDelArchivoDeSalida[50]={0};
   Nodo *arbol; //arbol de codificación
   long int longitud; //la longitud del archivo
   int num_elementos;  //número de elementos del árbol
   FILE *fe, *fs; //ficheros de entrada y salida

   unsigned char a;
   Nodo *p, *q;
   int i, j;

   printf("Ingese el archivo codificado: ");
   scanf("%s",nombreDelArchivoDeEntrada);

   printf("\n¿Como desea guardar el archivo original? Ej. ArchivoOriginal.txt: ");
   scanf("%s",nombreDelArchivoDeSalida);

   // Measure start time
    struct timeval ti;
    gettimeofday(&ti, NULL);
    double start = ti.tv_sec * 1000 + ti.tv_usec / 1000;

   //crear arbol basado en la información de la tabla
   arbol = (Nodo *)malloc(sizeof(Nodo)); /* un nodo nuevo */
   arbol->caracter = 0;
   arbol->uno = arbol->cero = NULL;
   fe = fopen(nombreDelArchivoDeEntrada, "rb");
   fread(&longitud, sizeof(long int), 1, fe); //lee el numero de caracteres
   fread(&num_elementos, sizeof(int), 1, fe); //lee el num de elementos
  
   for(i = 0; i < num_elementos; i++) //recorre todos los elementos
   {
      p = (Nodo *)malloc(sizeof(Nodo)); //nodo nuevo
      fread(&p->caracter, sizeof(char), 1, fe); //se lee el carater
      fread(&p->bits, sizeof(unsigned long int), 1, fe); //lee su correspondiente código
      fread(&p->num_bits, sizeof(char), 1, fe); //lee su lingitud
      p->cero = p->uno = NULL;
      //e inserta el nodo donde corresponde
      j = 1 << (p->num_bits-1);
      q = arbol;
      while(j > 1)
      {
         if(p->bits & j)
            if(q->uno) q = q->uno; //si el nodo existe, nos movemos ahi
            else //si no, se creará
            {
               q->uno = (Nodo *)malloc(sizeof(Nodo));
               q = q->uno;
               q->caracter = 0;
               q->uno = q->cero = NULL;
            }
         else //si es cero
            if(q->cero) q = q->cero; //si existe nos movemos a el 
            else //si no, se creará
            {
               q->cero = (Nodo *)malloc(sizeof(Nodo));
               q = q->cero;
               q->caracter = 0;
               q->uno = q->cero = NULL;
            }
         j >>= 1; //siguiente bit
      }
      
      if(p->bits & 1) //si el ultimo bit es un uno
         q->uno = p;
      else // o si es cero
         q->cero = p;
   }
   
   //leer datos comprimidos y extraer al archivo de salida
   bits = 0;
   fs = fopen(nombreDelArchivoDeSalida, "w");
   fread(&a, sizeof(char), 1, fe);
   bits |= a;
   bits <<= 8;
   fread(&a, sizeof(char), 1, fe);
   bits |= a;
   bits <<= 8;
   fread(&a, sizeof(char), 1, fe);
   bits |= a;
   bits <<= 8;
   fread(&a, sizeof(char), 1, fe);
   bits |= a;
   j = 0; //cada 8 bits se lee otro byte
   q = arbol;

   do {
      if(bits & 0x80000000) 
         q = q->uno; 
      else 
         q = q->cero; 
      bits <<= 1;  //siguiente bit
      j++;
      if(8 == j) //cada 8
      {
         i = fread(&a, sizeof(char), 1, fe); //se lee el byte desde el archivo
         bits |= a; //y se inserta en los bits
         j = 0; 
      }                                
      if(!q->uno && !q->cero) //si el nodo es un caracter
      {
         putc(q->caracter, fs); //se escribe en el archivo de salida
         longitud--;  //se actualiza la longitud restante
         q=arbol; //volver a raiz del árbol
      }
   } while(longitud); //hasta que acabe de recorrer el archivo

   printf("\nArchivo decodificado correctamente.\n");

   fclose(fs);     
   fclose(fe);

   borrarArbol(arbol);  

   // Measure end time
    gettimeofday(&ti, NULL);
    double end = ti.tv_sec * 1000 + ti.tv_usec / 1000;

    // Elapsed time
    printf("%lf milliseconds elapsed\n", (end - start));
   return 0;
}

// Función recursiva para borrar un arbol
void borrarArbol(Nodo *n)
{
   if(n->cero) 
      borrarArbol(n->cero);
   if(n->uno)  
      borrarArbol(n->uno);
   free(n);
}