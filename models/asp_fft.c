/* Copyright (c) 2005, 2006, 2022 Alexey Ozeritsky
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define _USE_MATH_DEFINES
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "asp_fft.h"

#define SQRT_M_1_PI 0.56418958354775629

fft * FFT_init(int type, int N)
{
	fft * ret;
	int _n  = N;
	int deg = 0;
	int m;

	while (_n % 2 == 0) {
		_n /= 2; deg++;
	}

	if (!((1 << deg) == N))
		return 0;

	ret = (fft*)malloc(sizeof(fft));

	ret->N     = N;
	ret->n     = deg;
	ret->type  = type;
	ret->ffCOS = 0;
	ret->ffSIN = 0;
	ret->k     = 1;
	ret->sz    = 2 * N;

	if (type & FFT_COS) {
		ret->ffCOS = (double*)malloc(2 * N * sizeof(double));
		for (m = 0; m < 2 * N; ++m) {
			ret->ffCOS[m] = cos(m * M_PI / (double)N);
		}
	}

	if (type & FFT_SIN) {
		ret->ffSIN = (double*)malloc(2 * N * sizeof(double));
		for (m = 0; m < 2 * N; ++m) {
			ret->ffSIN[m] = sin(m * M_PI / (double)N);
		}
	}

	//if (type & FFT_PERIODIC) {
	//	ret->k = 2;
	//}

	return ret;
}

void FFT_free(fft * s)
{
	if (s->ffSIN) free(s->ffSIN);
	if (s->ffCOS) free(s->ffCOS);
	free(s);
}

      /** обратное использование:
        dx = d_la * sqrt(M_1_PI)  // шаг * корень из 2/_длина_отрезка(на сфере 2pi)
        pFFT_1(s, S, dx, n_la, fft);
        прямое использование:
        dx = d_la * sqrt(M_1_PI) // корень из 2/_длина_отрезка(на сфере 2pi)
        pFFT(S, s, dx, n_la, fft);
      */

	/*!быстрое преобразование Фурье периодической функции.
	   по значениям функции находим коэфф Фурье.
	   f(i)->fk
       Самарский-Николаев, страница 180-181, формулы 65-66
	  \param S  - ответ
	  \param s  - начальное условие
	  \param dx - множитель перед суммой
	  \param N  - число точек
	  \param n  - log2(N)
	*/
	void pFFT_1(double *S, double *s1, double dx, int N, int n)
	{
		int p, s, j, idx, idx2, k;
		int N_2 = N / 2;
		double *a   = (double*)malloc((n + 1)*N * sizeof(double));
		double * y  = (double*)malloc((N_2 + 1) * sizeof(double));
		double *_y  = (double*)malloc((N_2 + 1) * sizeof(double));

		memcpy(a, s1, N * sizeof(double));

		for (p = 1; p <= n; p++) {
			//int idx = _ipow(2, n - p);
			int idx = 1 << (n - p);
			for (j = 0; j <= idx - 1; j++) {
				a[p * N + j]       = a[(p - 1) * N + j] + a[(p - 1) * N + idx + j];
				a[p * N + idx + j] = a[(p - 1) * N + j] - a[(p - 1) * N + idx + j];
			}
		}

		for (s = 1; s <= n - 2; s++) {
			//idx = _ipow(2, n - s - 1);
			//vm  = _ipow(2, s);
			idx = 1 << (n - s - 1);
			//vm  = 1 << s;
			for (k = 1; k <= idx; k++) {
				double s1 = 0.0;
				double s2 = 0.0;
				for (j = 0; j <= 2 * idx - 1; j++) {
					s1 += a[s * N + 2 * idx + j] *
						cos((2 * k - 1) * M_PI * j / (double)(2 * idx));
						//ffCOS[(2 * k - 1) * vm * n_la + j];
				}
				for (j = 1; j <= 2 * idx - 1; j++) {
					s2 += a[s * N + 2 * idx + j] *
						sin((2 * k - 1) * M_PI * j / (double)(2 * idx));
						//ffSIN[(2 * k - 1) * vm * n_la + j];
				}
				//idx2 = _ipow(2, s - 1) * (2 * k - 1);
				idx2 = (1 << (s - 1)) * (2 * k - 1);
				y[idx2]  = s1;
				_y[idx2] = s2;
			}
		}

		y[N_2]              = a[n * N + 1];
		//y[_ipow(2, n - 2)]  = a[(n - 1) * N + 2];
		y[1 << (n - 2)] = a[(n - 1) * N + 2];

		y[0]                = a[n * N + 0];
		//_y[_ipow(2, n - 2)] = a[(n - 1) * N + 3];
		_y[1 << (n - 2)] = a[(n - 1) * N + 3];

		for (k = 0; k <= N_2; k++) {
			S[k] = y[k] * dx;
		}
		for (k = 1; k <= N_2 - 1; k++) {
			S[N - k] = _y[k] * dx;
		}

		free(a); free(_y); free(y);
	}

	void pFFT_2_1(double *S, const double *s1, double dx, fft * ft)
	{
		int p, s, j, idx, idx2, vm, k;
		int N = ft->N, n = ft->n, sz = ft->sz;
		double * ffCOS = ft->ffCOS, * ffSIN = ft->ffSIN;
		int N_2 = N / 2;
		double *a   = (double*)malloc((n + 1)*N * sizeof(double));
		double * y  = (double*)malloc((N_2 + 1) * sizeof(double));
		double *_y  = (double*)malloc((N_2 + 1) * sizeof(double));

		memcpy(a, s1, N * sizeof(double));

		for (p = 1; p <= n; p++) {
			//int idx = _ipow(2, n - p);
			idx = 1 << (n - p);
			for (j = 0; j <= idx - 1; j++) {
				a[p * N + j]       = a[(p - 1) * N + j] + a[(p - 1) * N + idx + j];
				a[p * N + idx + j] = a[(p - 1) * N + j] - a[(p - 1) * N + idx + j];
			}
		}

		for (s = 1; s <= n - 2; s++) {
			//idx = _ipow(2, n - s - 1);
			//vm  = _ipow(2, s);
			idx = 1 << (n - s - 1);
			vm  = 1 << s;
			for (k = 1; k <= idx; k++) {
				double s1 = 0.0;
				double s2 = 0.0;
				for (j = 0; j <= 2 * idx - 1; j++) {
					s1 += a[s * N + 2 * idx + j] *
						//cos((2 * k - 1) * M_PI * j / (double)(2 * idx));
						//ffCOS[(2 * k - 1) * vm * n_la + j];
						ffCOS[((2 * k - 1) * vm * j) % sz];
				}
				for (j = 1; j <= 2 * idx - 1; j++) {
					s2 += a[s * N + 2 * idx + j] *
						//sin((2 * k - 1) * M_PI * j / (double)(2 * idx));
						//ffSIN[(2 * k - 1) * vm * n_la + j];
						ffSIN[((2 * k - 1) * vm * j) % sz];
				}
				//idx2 = _ipow(2, s - 1) * (2 * k - 1);
				idx2 = (1 << (s - 1)) * (2 * k - 1);
				y[idx2]  = s1;
				_y[idx2] = s2;
			}
		}

		y[N_2]              = a[n * N + 1];
		//y[_ipow(2, n - 2)]  = a[(n - 1) * N + 2];
		y[1 << (n - 2)]  = a[(n - 1) * N + 2];

		y[0]                = a[n * N + 0];
		//_y[_ipow(2, n - 2)] = a[(n - 1) * N + 3];
		_y[1 << (n - 2)] = a[(n - 1) * N + 3];

		for (k = 0; k <= N_2; k++) {
			S[k]     = y[k] * dx;
		}
		for (k = 1; k <= N_2 - 1; k++) {
			S[N - k] = _y[k] * dx;
		}

		free(a); free(_y); free(y);
	}

	/*!быстрое преобразование Фурье периодической функции.
	   по коэфф Фурье находим значения функции.
	   fk->f(i)
       Самарский-Николаев, страница 180-181
	  \param S  - ответ
	  \param s  - начальное условие
	  \param dx - множитель перед суммой
	  \param N  - число точек
	  \param n  - log2(N)
	*/
	void pFFT(double *S, double *s, double dx, int N, int n) {
		int N_2 = N/2;
		int k;
		double *y1 = (double*)malloc((N_2 + 1) * sizeof(double));
		double *y2 = (double*)malloc((N_2 + 1) * sizeof(double));
		double *ss = (double*)malloc((N_2 + 1) * sizeof(double));

		memcpy(&ss[0], &s[0], (N_2+1) * sizeof(double));

		cFFT(y1, ss, dx, N/2, n - 1);

		for (k = 1; k <= N_2-1; k++) {
			ss[k] = s[N-k];
		}
		sFFT(y2, ss, dx, N/2, n - 1);

		for (k = 1; k <= N_2 - 1; k ++) {
			S[k]   = (y1[k] + y2[k]);
			S[N-k] = (y1[k] - y2[k]);
		}
		S[0]   = y1[0]  ;
		S[N_2] = y1[N_2];
		free(y1); free(y2); free(ss);
	}

	void pFFT_2(double *S, double *s, double dx, fft * ft)
	{
		int N = ft->N;
		int N_2 = N/2;
		int k;
		double *y1 = (double*)malloc((N_2 + 1) * sizeof(double));
		double *y2 = (double*)malloc((N_2 + 1) * sizeof(double));
		double *ss = (double*)malloc((N_2 + 1) * sizeof(double));

		fft ft2 = *ft;
		memcpy(&ss[0], &s[0], (N_2+1) * sizeof(double));

		//ft->N = N_2; --ft->n; ft->k = 2;
		ft2.N = N_2;
		ft2.n = ft->n - 1;
		ft2.k = 2;

		cFFT_2(y1, ss, dx, &ft2);

		for (k = 1; k <= N_2-1; k++) {
			ss[k] = s[N-k];
		}
		sFFT_2(y2, ss, dx, &ft2);

		//ft->N = N; ++ft->n; ft->k = 1;

		for (k = 1; k <= N_2 - 1; k ++) {
			S[k]   = (y1[k] + y2[k]);
			S[N-k] = (y1[k] - y2[k]);
		}
		S[0]   = y1[0]  ;
		S[N_2] = y1[N_2];
		free(y1); free(y2); free(ss);
	}

	/*!медленное синусное преобразование*/
	void sFT(double *S, double *s, double dx, int N)
	{
		int j, k;
		for (k = 1; k <= N - 1; ++k) {
			double sum = 0.0;
			for (j = 1; j <= N-1; ++j) {
				sum += s[j] * sin(k * M_PI * j / ((double)N));
			}
			S[k] = dx * sum;
		}
	}

	/*! быстрое синусное преобразование.
	   Самарский-Николаев, страница 180
	 */
	void sFFT(double *S, double *ss, double dx, int N, int n) {
		int p, s, k, j, idx;
		double * a = (double*)malloc(n * N * sizeof(double));
		memcpy(&a[1], &ss[1], (N - 1) * sizeof(double));

		for (p = 1; p <= n - 1; p++) {
			//int idx  = _ipow(2, n - p);
			int idx = 1 << (n - p);
			for (j = 1; j <= idx - 1; j ++) {
				a[p * N + j]           = a[(p - 1) * N + j] - a[(p - 1) * N + 2 * idx - j];
				a[p * N + 2 * idx - j] = a[(p - 1) * N + j] + a[(p - 1) * N + 2 * idx - j];
				a[p * N + idx]         = a[(p - 1) * N + idx];
			}
		}

		for (s = 1; s <= n - 1; s++) {
			//int idx = _ipow(2, n - s);
			//int vm  = _ipow(2, s - 1);
			int idx = 1 << (n - s);
			int vm  = 1 << (s - 1);
			for (k = 1; k <= idx; k++) {
				double y = 0.0;
				for (j = 1; j <= idx; j++) {
					y += a[s * N + idx * 2 - j] *
						sin((2 * k - 1) * M_PI * j / (double)(idx * 2));
						//ffSIN[(2 * k - 1) * vm * 2 * n_la + j];

				}
				S[(2 * k - 1) * vm] = y * dx;
			}
		}
		//idx = _ipow(2, n - 1);
		idx = 1 << (n - 1);
		S[idx] = a[(n - 1) * N + 1] * dx;// * vm_mult;
		free(a);
	}

	/*! быстрое синусное преобразование.
	   Самарский-Николаев, страница 180
	 */
	void sFFT_2(double *S, double *ss, double dx, fft * ft) {
		int p, s, k, j, idx;
		int n = ft->n, N = ft->N, nr = ft->k, sz = ft->sz;
		double * ffSIN = ft->ffSIN;
		double * a = (double*)malloc(n * N * sizeof(double));
		memcpy(&a[1], &ss[1], (N - 1) * sizeof(double));

		for (p = 1; p <= n - 1; p++) {
			//int idx  = _ipow(2, n - p);
			int idx = 1 << (n - p);
			for (j = 1; j <= idx - 1; j ++) {
				a[p * N + j]           = a[(p - 1) * N + j] - a[(p - 1) * N + 2 * idx - j];
				a[p * N + 2 * idx - j] = a[(p - 1) * N + j] + a[(p - 1) * N + 2 * idx - j];
				a[p * N + idx]         = a[(p - 1) * N + idx];
			}
		}

		for (s = 1; s <= n - 1; s++) {
			//int idx = _ipow(2, n - s);
			//int vm  = _ipow(2, s - 1);
			int idx = 1 << (n - s);
			int vm  = 1 << (s - 1);
			for (k = 1; k <= idx; k++) {
				double y = 0.0;
				for (j = 1; j <= idx; j++) {
					y += a[s * N + idx * 2 - j] *
						//sin((2 * k - 1) * M_PI * j / (double)(idx * 2));
						//ffSIN[(2 * k - 1) * vm * 2 * n_la + j];
						ffSIN[((2 * k - 1) * vm * nr * j) % sz];

				}
				S[(2 * k - 1) * vm] = y * dx;
			}
		}
		//idx = _ipow(2, n - 1);
		idx = 1 << (n - 1);
		S[idx] = a[(n - 1) * N + 1] * dx;// * vm_mult;
		free(a);
	}

	/*!медленное косинусное преобразование*/
	void cFT(double *S, double *s, double dx, int N)
	{
		int j, k;
        int _cos = 1;
		for (k = 0; k <= N; ++k) {
			double sum = 0.0;
            sum += 0.5*s[0] * 1; /*cos(k * M_PI * 0 / ((double)N))*/;
			for (j = 1; j < N; ++j) {
				sum += s[j] * cos(k * M_PI * j / ((double)N));
			}
            sum += 0.5*s[N] * _cos; /*cos(k * M_PI * N / ((double)N))*/;
			S[k] = dx * sum; _cos *= -1;
		}
	}

	/*! быстрое косинусное преобразование.
	   Самарский-Николаев, страница 176, формулы 46-47
	 */
	void cFFT(double *S, double *ss, double dx, int N, int n) {
		int p, s, j, k;
		int M = N + 1;
		double *a = (double*)malloc((n + 1) * M * sizeof(double));

		memcpy(&a[0], &ss[0], M * sizeof(double));
        a[0] *= 0.5; a[N] *= 0.5; // samarskii, (15)-(16) p 66

		for (p = 1; p <= n; p++) {
			//int idx  = _ipow(2, n - p);
			int idx = 1 << (n - p);
			for (j = 0; j <= idx - 1; j ++) {
				a[p * M + j]           = a[(p - 1) * M + j] + a[(p - 1) * M + 2 * idx - j];
				a[p * M + 2 * idx - j] = a[(p - 1) * M + j] - a[(p - 1) * M + 2 * idx - j];
				a[p * M + idx]         = a[(p - 1) * M + idx];
			}
		}

		for (s = 1; s <= n - 1; s++) {
			//int idx = _ipow(2, n - s);
			//int vm  = _ipow(2, s - 1);
			int idx = 1 << (n - s);
			int vm  = 1 << (s - 1);
			for (k = 1; k <= idx; k++) {
				double y = 0.0;
				for (j = 0; j <= idx - 1; j++) {
					y += a[s * M + idx * 2 - j] *
						cos((2 * k - 1) * M_PI * j / (double)(idx * 2));
						//ffCOS[(2 * k - 1) * vm * 2 * n_la + j];
				}
				S[(2 * k - 1) * vm] = y * dx;
			}
		}
		S[0]   = (a[n * M + 0] + a[n * M + 1]) * dx;
		S[N]   = (a[n * M + 0] - a[n * M + 1]) * dx;
		S[N/2] =  a[n * M + 2] * dx;
		free(a);
	}

	void cFFT_2(double *S, double *ss, double dx, fft * ft)
	{
		int p, s, j, k;
		int n = ft->n, N = ft->N, nr = ft->k, sz = ft->sz;
		double * ffCOS = ft->ffCOS;
		int M = N + 1;
		double *a = (double*)malloc((n + 1) * M * sizeof(double));

		memcpy(&a[0], &ss[0], M * sizeof(double));
        a[0] *= 0.5; a[N] *= 0.5; // samarskii, (15)-(16) p 66

		for (p = 1; p <= n; p++) {
			//int idx  = _ipow(2, n - p);
			int idx = 1 << (n - p);
			for (j = 0; j <= idx - 1; j ++) {
				a[p * M + j]           = a[(p - 1) * M + j] + a[(p - 1) * M + 2 * idx - j];
				a[p * M + 2 * idx - j] = a[(p - 1) * M + j] - a[(p - 1) * M + 2 * idx - j];
				a[p * M + idx]         = a[(p - 1) * M + idx];
			}
		}

		for (s = 1; s <= n - 1; s++) {
			//int idx = _ipow(2, n - s);
			//int vm  = _ipow(2, s - 1);
			int idx = 1 << (n - s);
			int vm  = 1 << (s - 1);
			for (k = 1; k <= idx; k++) {
				double y = 0.0;
				for (j = 0; j <= idx - 1; j++) {
					y += a[s * M + idx * 2 - j] *
						//cos((2 * k - 1) * M_PI * j / (double)(idx * 2));
						//ffCOS[(2 * k - 1) * vm * 2 * n_la + j];
						ffCOS[((2 * k - 1) * vm * nr * j) % sz];
				}
				S[(2 * k - 1) * vm] = y * dx;
			}
		}
		S[0]   = (a[n * M + 0] + a[n * M + 1]) * dx;
		S[N]   = (a[n * M + 0] - a[n * M + 1]) * dx;
		S[N/2] =  a[n * M + 2] * dx;
		free(a);
	}


	/*!быстрое преобразование Фурье.
	   Самарский-Николаев, страница 170, формулы 30-31
	  \param S  - ответ
	  \param s  - начальное условие
	  \param dx - множитель перед суммой
	      d_y * sqrt(2 / l_y) для преобразования значений функции в коэф Фурье
		и sqrt(2 / l_y) для нахождения функции по коэф Фурье
	  \param Vm - собственные функции в виде
	      Vm(m, j) = sqrt(2. / l_y) * sin(M_PI * m * j / n_y)
		  где l_y - длина отрезка, n_y - число точек
	  \param N  - число точек
	  \param n  - log2(N)
	*/
	void FFT(double *S, const double *ss, double *Vm, double dx, int N, int n)
	{
		int s, k, j, p, idx;
		double *a = (double*)malloc(n * N * sizeof(double));
		memcpy(&a[1], &ss[1], (N - 1) * sizeof(double));

		for (p = 1; p <= n - 1; p++) {
			//idx  = _ipow(2, n - p);
			idx = 1 << (n - p);
			for (j = 1; j <= idx - 1; j ++) {
				a[p * N + j]           = a[(p - 1) * N + j] - a[(p - 1) * N + 2 * idx - j];
				a[p * N + 2 * idx - j] = a[(p - 1) * N + j] + a[(p - 1) * N + 2 * idx - j];
				a[p * N + idx]         = a[(p - 1) * N + idx];
			}
		}

		for (s = 1; s <= n - 1; s++) {
			//int idx = _ipow(2, n - s);
			//int vm  = _ipow(2, s - 1);
			int idx = 1 << (n - s);
			int vm  = 1 << (s - 1);
			for (k = 1; k <= idx; k++) {
				double y = 0.0;
				for (j = 1; j <= idx; j++) {
					y += a[s * N + idx * 2 - j] * Vm[(2 * k - 1) * vm * N + j];
				}
				S[(2 * k - 1) * vm] = y * dx;
			}
		}
		//idx = _ipow(2, n - 1);
		idx = 1 << (n - 1);
		S[idx] = a[(n - 1) * N + 1] * dx;
		free(a);
	}
