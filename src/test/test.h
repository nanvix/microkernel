/*
 * MIT License
 *
 * Copyright(c) 2011-2020 The Maintainers of Nanvix
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _TEST_H_
#define _TEST_H_

	/**
	 * @brief Number of iterations in stress tests.
	 */
	#define NITERATIONS 10

	/**
	 * @brief Number of threads to spawn in stress tests.
	 */
	#define NTHREADS (THREAD_MAX - 1)

	#define ___STRINGIFY(x) #x
	#define ___TOSTRING(x) ___STRINGIFY(x)

	/**
	 * @brief Unit test.
	 */
	struct test
	{
		void (*test_fn)(void); /**< Test function. */
		const char *name;      /**< Test name.     */
	};

	/**
	 * @brief Writes a string to the standard output device.
	 *
	 * @param str Target string.
	 */
	extern void nanvix_puts(const char *str);

	/**
	 * @name User-Level Testing Units
	 */
	/**@{*/
	extern void test_network(void);

	extern void test_freeze(void);

	/**@}*/

	/**
	 * @brief Asserts a condition.
	 *
	 * @param x Condition to assert.
	 */
	#define test_assert(x)                            \
	{                                                 \
		if (!(x))                                     \
		{                                             \
			nanvix_puts("assertation failed at "      \
				__FILE__":" ___TOSTRING(__LINE__)"\n" \
			);                                        \
			kshutdown() /* noop */;                   \
		}                                             \
	}

#endif /* _TEST_H_  */
