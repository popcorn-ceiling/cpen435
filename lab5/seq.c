/* filename: seq.c
 * author: daniel collins (dcollins3@zagmail.gonzaga.edu)
 * date: 3/27/15
 * brief: quicksort and bubble sort includes for lab5
 */

#include <stdio.h>

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

void bubblesort (int *a, int n) {
    int i, t, s = 1;
    while (s) {
        s = 0;
        for (i = 1; i < n; i++) {
            if (a[i] < a[i - 1]) {
                t = a[i];
                a[i] = a[i - 1];
                a[i - 1] = t;
                s = 1;
            }
        }
    }
}

/* will's sorts */
void wbubblesort(int *a, int n) {
    int i, j, tmp;
    for (i = 0; i < n; i++) {
        for (j = 0; j < n-1; j++) {
            if (a[j] > a[j+1]) {
                tmp = a[j+1];
                a[j+1] = a[j];
                a[j] = tmp;
            }
        }
    }
}


void wquicksort(int *array, int lo, int hi, int flag)
{
    int i = lo-1;
    int j = hi;
    int pivot= array[hi];
    int temp;
    if (hi>lo) {
        do {
            if(flag == 1) /* a increase sort */ {
                do i++; while (array[i]<pivot);
                do j--; while (array[j]>pivot);
            }
            else /* a decrease sort */ {
                do i++; while (array[i]>pivot);
                do j--; while (array[j]<pivot);
            }
            temp = array[i]; /* swap values */
            array[i] = array[j];
            array[j] = temp;
        } while (j>i);

        array[j] = array[i]; /* swap values */
        array[i] = pivot;
        array[hi] = temp;
        wquicksort(array,lo,i-1,flag); /* recursive until hi == lo */
        wquicksort(array,i+1,hi,flag);
    }
} 
