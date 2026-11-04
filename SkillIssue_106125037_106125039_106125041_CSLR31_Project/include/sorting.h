/**
 * sorting.h - Generic merge sort used for ranking (module: Sorting).
 *
 * Merge sort is used (instead of std::sort) because it is STABLE and runs in
 * O(n log n) in every case. Stability keeps equal-ranked users in a
 * predictable order. The comparator decides the ordering, e.g.
 * "higher teachingPoints first, ties broken by smaller ID".
 *
 * Used by: Platform::getLeaderboardAPI, getMatchingMentorsAPI, bookSession.
 */
#ifndef SKILL_ISSUE_SORTING_H
#define SKILL_ISSUE_SORTING_H

#include <vector>

/**
 * Recursively sorts arr[left..right] (inclusive) using the comparator.
 * comp(a, b) must return true when a should come BEFORE b.
 */
template <typename T, typename Compare>
void mergeSortHelper(std::vector<T>& arr, int left, int right, Compare comp) {
    if (left >= right) return;              // 0 or 1 element: already sorted

    int mid = left + (right - left) / 2;    // avoids integer overflow

    mergeSortHelper(arr, left, mid, comp);      // sort left half
    mergeSortHelper(arr, mid + 1, right, comp); // sort right half

    // Merge the two sorted halves into a temporary buffer.
    std::vector<T> temp;
    int i = left;
    int j = mid + 1;

    while (i <= mid && j <= right) {
        // Take from the left half unless the right element must come first.
        // Using !comp(right, left) preserves the original order of ties.
        if (!comp(arr[j], arr[i])) {
            temp.push_back(arr[i]);
            i++;
        } else {
            temp.push_back(arr[j]);
            j++;
        }
    }

    while (i <= mid)   temp.push_back(arr[i++]);   // leftovers, left half
    while (j <= right) temp.push_back(arr[j++]);   // leftovers, right half

    // Copy the merged result back into the original range.
    for (int k = 0; k < static_cast<int>(temp.size()); k++)
        arr[left + k] = temp[k];
}

/** Public entry point: sorts the whole vector in place. */
template <typename T, typename Compare>
void mergeSort(std::vector<T>& arr, Compare comp) {
    if (arr.size() > 1)
        mergeSortHelper(arr, 0, static_cast<int>(arr.size()) - 1, comp);
}

#endif // SKILL_ISSUE_SORTING_H
