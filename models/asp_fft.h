#ifndef _ASP_FFT_H
#define _ASP_FFT_H
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

/**
 * Быстрое преобразование Фурье
 *
 */

//использовать библиотеку fftw?
//warning: библиотека под GPL лицензией!
//для коммерческих разработок надо убрать этот define !
//#define _FFTW

	enum FFT_type {
		FFT_SIN      = 1, //!<синусное преобразование
		FFT_COS      = 2, //!<косинусное преобразование
		FFT_PERIODIC = 3  //!<периодическое (полсуммы по син и пол по кос) FFT_SIN & FFT_COS
	};

	typedef struct _fft_internal {
		int type; //!<тип преобразования
		int N;    //!<размерность задачи
		int sz;   //!<размер таблиц с sin/cos
		int n;    //!<степень двойки
		int k;    //!<коэф-т нормировки внутри cos/sin (нужно для PERIODIC)
		double * ffCOS;
		double * ffSIN;
	} fft;

	//!инициализирует структуру
	fft * FFT_init(int type, int N);
	void FFT_free(fft *);

/*~~~ значения синусов и косинусов вычисляются в функциях ~~~~~ */
/*~~~ sin((2 * k - 1) * M_PI * j / (double)(idx * 2)); ~~~~~~~~ */
/*~~~ cos((2 * k - 1) * M_PI * j / (double)(idx * 2)); ~~~~~~~~ */
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
	void pFFT(double *S, double *s, double dx, int N, int n);
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
	void pFFT_1(double *S, double *s1, double dx, int N, int n);

	/*! быстрое синусное преобразование.
	   Самарский-Николаев, страница 180
	 */
	void sFFT(double *S, double *s, double dx, int N, int n);
	/*!медленное синусное преобразование*/
	void sFT(double *S, double *s, double dx, int N);

	/*! быстрое косинусное преобразование.
	   Самарский-Николаев, страница 176, формулы 46-47
	 */
	void cFFT(double *S, double *s, double dx, int N, int n);
	/*!медленное косинусное преобразование*/
	void cFT(double *S, double *s, double dx, int N);

/*~~~ значения синусов и косинусов даны ~~~~~ */
/*вместо sin((2 * k - 1) * M_PI * j / (double)(idx * 2));
  используем массив
  ffSIN[(2 * k - 1) * vm * 2 * n_la + j];
  вместо cos((2 * k - 1) * M_PI * j / (double)(idx * 2));
  используем массив
  ffCOS[(2 * k - 1) * vm * 2 * n_la + j];
  где, значения берутся так в случае сетки на сфере:
  (в случаях других сеток будет отличаться нормировка
  под sin, cos)

			ffCOS = new double[n_la * n_la];
			ffSIN = new double[n_la * n_la];

			for (int m = 0; m < n_la; m++) {
				for (int j = 0; j < n_la; j++) {
					ffCOS[m * n_la + j] =
						cos(m * M_PI * j / (double)n_la);
					ffSIN[m * n_la + j] =
						sin(m * M_PI * j / (double)n_la);
				}
			}
*/
	/*!быстрое преобразование Фурье периодической функции.
	   по коэфф Фурье находим значения функции.
	   fk->f(i)
       Самарский-Николаев, страница 180-181
	  \param S  - ответ
	  \param s  - начальное условие
	  \param dx - множитель перед суммой
	  \param p  - структура, проинициализированная FFT_init
	*/
	void pFFT_2(double *S, double *s, double dx, fft * p);

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
	  \param p  - структура, проинициализированная FFT_init
	*/
	void pFFT_2_1(double *S, const double *s1, double dx, fft * p);

	/*! быстрое синусное преобразование.
	   Самарский-Николаев, страница 180
	 */
	void sFFT_2(double *S, double *s, double dx, fft * p);

	/*! быстрое косинусное преобразование.
	    Самарский-Николаев, страница 176, формулы 46-47
	 */
	void cFFT_2(double *S, double *s, double dx, fft * p);

	/*!быстрое преобразование Фурье.
	   Самарский-Николаев, страница 170, формулы 30-31
	  \param S  - ответ
	  \param s  - начальное условие
	  \param dx - множитель перед суммой
	      d_y * sqrt(2 / l_y) для преобразования значений функции в коэф Фурье
		и sqrt(2 / l_y) для нахождения функции по коэф Фурье
	  \param Vm - собственные функции в виде
	      Vm(m, j) = sqrt(2. / ly) * sin(M_PI * m * j / n_y)
		  где l_y - длина отрезка, n_y - число точек
	  \param N  - число точек
	  \param n  - log2(N)
	*/
	void FFT(double *S, const double *s, double *Vm, double dx, int N, int n);

#endif //_ASP_FFT_H
