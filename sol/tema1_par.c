/*
 * APD - Tema 1
 * Octombrie 2020
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>

char *in_filename_julia;
char *in_filename_mandelbrot;
char *out_filename_julia;
char *out_filename_mandelbrot;

// Structura pentru un numar complex
typedef struct _complex {
	double a;
	double b;
} complex;

// Structura pentru parametrii unei rulari
typedef struct _params {
	int is_julia, iterations;
	double x_min, x_max, y_min, y_max, resolution;
	complex c_julia;
} params;

// Parametrii unei rulari
params par;
// Matricea in care se retine rezultatul
int **result;
// Lungimea si latimea matricei
int width, height;
// Numarul de threaduri
int num_threads;
// Bariera pentru sincronizarea threadurilor
pthread_barrier_t barrier;

// Citeste argumentele programului
void get_args(int argc, char **argv)
{
	if (argc < 5) {
		printf("Numar insuficient de parametri:\n\t"
				"./tema1 fisier_intrare_julia fisier_iesire_julia "
				"fisier_intrare_mandelbrot fisier_iesire_mandelbrot\n");
		exit(1);
	}

	in_filename_julia = argv[1];
	out_filename_julia = argv[2];
	in_filename_mandelbrot = argv[3];
	out_filename_mandelbrot = argv[4];
}

// Citeste fisierul de intrare
void read_input_file(char *in_filename, params* par)
{
	FILE *file = fopen(in_filename, "r");
	if (file == NULL) {
		printf("Eroare la deschiderea fisierului de intrare!\n");
		exit(1);
	}

	fscanf(file, "%d", &par->is_julia);
	fscanf(file, "%lf %lf %lf %lf",
			&par->x_min, &par->x_max, &par->y_min, &par->y_max);
	fscanf(file, "%lf", &par->resolution);
	fscanf(file, "%d", &par->iterations);

	if (par->is_julia) {
		fscanf(file, "%lf %lf", &par->c_julia.a, &par->c_julia.b);
	}

	fclose(file);
}

// Scrie rezultatul in fisierul de iesire
void write_output_file(char *out_filename, int **result, int width, int height)
{
	int i, j;

	FILE *file = fopen(out_filename, "w");
	if (file == NULL) {
		printf("Eroare la deschiderea fisierului de iesire!\n");
		return;
	}

	fprintf(file, "P2\n%d %d\n255\n", width, height);
	for (i = 0; i < height; i++) {
		for (j = 0; j < width; j++) {
			fprintf(file, "%d ", result[i][j]);
		}
		fprintf(file, "\n");
	}

	fclose(file);
}

void* run_julia_and_mandelbrot(void* arg) {
	// ID-ul threadului luat din argument
	int thread_id = *(int*)arg;
	int w, h, i;

	if (thread_id == 0) {
		// Se citesc parametrii de intrare pentru algoritmul Julia
		read_input_file(in_filename_julia, &par);

		// Se aloca tabloul cu rezultatul
		width = (par.x_max - par.x_min) / par.resolution;
		height = (par.y_max - par.y_min) / par.resolution;
		result = malloc(height * sizeof(int*));
		
		if (result == NULL) {
			printf("Eroare la malloc!\n");
			exit(1);
		}
	}

	pthread_barrier_wait(&barrier);

	// Indecsii start si end pentru alocarea matricei
	int start = thread_id * (double) height / num_threads;
	int end = fmin((thread_id + 1) * (double) height / num_threads, height);

	for (i = start; i < end; i++) {
		result[i] = malloc(width * sizeof(int));
		if (result[i] == NULL) {
			printf("Eroare la malloc!\n");
			exit(1);
		}
	}

	pthread_barrier_wait(&barrier);

	// Pozitiile de start si end pentru algoritmul Julia
	start = thread_id * (double) width / num_threads;
	end = fmin((thread_id + 1) * (double) width / num_threads, width);

	// Algoritmul Julia
	for (w = start; w < end; w++) {
		for (h = 0; h < height; h++) {
			int step = 0;
			complex z = { .a = w * par.resolution + par.x_min,
							.b = h * par.resolution + par.y_min };

			while (z.a * z.a + z.b * z.b < 4.0 && step < par.iterations) {
				complex z_aux = { .a = z.a, .b = z.b };

				z.a = z_aux.a * z_aux.a - z_aux.b * z_aux.b + par.c_julia.a;
				z.b = 2 * z_aux.a * z_aux.b + par.c_julia.b;

				step++;
			}

			result[height - h - 1][w] = step % 256;
		}
	}

	pthread_barrier_wait(&barrier);

	/*
	 * Cu ajutorul unui singur thread se scrie rezultatul algoritmului Julia, 
	 * se elibereaza memoria si se realoca din nou dupa citirea datelor de 
	 * intrare pentru fisierul Mandelbrot.
	 */
	if (thread_id == 0) {
		// Se scrie rezultatul algoritmului Julia
		write_output_file(out_filename_julia, result, width, height);
	}

	pthread_barrier_wait(&barrier);

	// Indecsii start si end pentru dezalocarea matricei
	start = thread_id * (double) height / num_threads;
	end = fmin((thread_id + 1) * (double) height / num_threads, height);

	for (i = start; i < end; i++) {
		free(result[i]);
	}

	pthread_barrier_wait(&barrier);

	if (thread_id == 0) {
		// Dezalocarea matricei
		free(result);

		// Se citesc parametrii de intrare pentru algoritmul Julia
		read_input_file(in_filename_mandelbrot, &par);
	
		// Se aloca matricea cu rezultatul
		width = (par.x_max - par.x_min) / par.resolution;
		height = (par.y_max - par.y_min) / par.resolution;
		result = malloc(height * sizeof(int*));

		if (result == NULL) {
			printf("Eroare la malloc!\n");
			exit(1);
		}
	}

	pthread_barrier_wait(&barrier);

	// Indecsii start si end pentru alocarea matricei
	start = thread_id * (double) height / num_threads;
	end = fmin((thread_id + 1) * (double) height / num_threads, height);

	for (i = start; i < end; i++) {
		result[i] = malloc(width * sizeof(int));
		if (result[i] == NULL) {
			printf("Eroare la malloc!\n");
			exit(1);
		}
	}

	pthread_barrier_wait(&barrier);

	// Pozitiile de start si end pentru algoritmul Mandelbrot
	start = thread_id * (double) width / num_threads;
	end = fmin((thread_id + 1) * (double) width / num_threads, width);

	// Algoritmul Mandelbrot
	for (w = start; w < end; w++) {
		for (h = 0; h < height; h++) {
			complex c = { .a = w * par.resolution + par.x_min,
							.b = h * par.resolution + par.y_min };
			complex z = { .a = 0, .b = 0 };
			int step = 0;

			while (z.a * z.a + z.b * z.b < 4.0 && step < par.iterations) {
				complex z_aux = { .a = z.a, .b = z.b };

				z.a = z_aux.a * z_aux.a - z_aux.b * z_aux.b + c.a;
				z.b = 2.0 * z_aux.a * z_aux.b + c.b;

				step++;
			}

			result[height - h - 1][w] = step % 256;
		}
	}

	pthread_barrier_wait(&barrier);

	if (thread_id == 0) {
		// Se scrie rezultatul algoritmului Mandebrot in fisierul de iesire
		write_output_file(out_filename_mandelbrot, result, width, height);
	}

	pthread_barrier_wait(&barrier);

	// Indecsii start si end pentru dezalocarea matricei
	start = thread_id * (double) height / num_threads;
	end = fmin((thread_id + 1) * (double) height / num_threads, height);

	for (i = start; i < end; i++) {
		free(result[i]);
	}

	pthread_barrier_wait(&barrier);

	if (thread_id == 0) {
		free(result);
	}

	pthread_barrier_wait(&barrier);

	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
	// Se citesc argumentele programului
	get_args(argc, argv);

	// Numarul de thread-uri, obtinut din argumentele programului
	num_threads = atoi(argv[5]);

	// Vectorii de argumente si thread-uri
	int arguments[num_threads];
	pthread_t threads[num_threads];

	// Se initializeaza bariera
	pthread_barrier_init(&barrier, NULL, num_threads);
	
	// Se creaza threadurile si se ruleaza algoritmul
	for (int id = 0; id < num_threads; ++id) {
		arguments[id] = id;
		pthread_create(&threads[id], NULL, run_julia_and_mandelbrot, &arguments[id]);
	}

	// Se distrug threadurile
	for (int id = 0; id < num_threads; ++id) {
		pthread_join(threads[id], NULL);
	}

	// Se elibereaza memoria alocata pentru bariera
	pthread_barrier_destroy(&barrier);

	return 0;
}
