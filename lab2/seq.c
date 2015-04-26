/* filename: seq.c
 * author: daniel collins (dcollins3@zagmail.gonzaga.edu)
 * date: 2/27/15
 * brief: quicksort which can be used by each thread to sort array locally
 */

#include <stdio.h>

/* used a different quicksort than the one in the lab2 doc */
void quicksort (int *a, int n) {
    int i, j, p, t;
    if (n < 2)
        return;

    p = a[n / 2];
    for (i = 0, j = n - 1;; i++, j--) {
        while (a[i] < p)
            i++;
        while (p < a[j])
            j--;
        if (i >= j)
            break;
        t = a[i];
        a[i] = a[j];
        a[j] = t;
    }
    quicksort(a, i);
    quicksort(a + i, n - i);
}

