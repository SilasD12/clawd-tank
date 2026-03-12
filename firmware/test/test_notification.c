// firmware/test/test_notification.c
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../main/notification.h"

static notification_store_t store;

static void test_init_empty(void) {
    notif_store_init(&store);
    assert(notif_store_count(&store) == 0);
    assert(notif_store_get(&store, 0) == NULL);
    printf("  PASS: test_init_empty\n");
}

static void test_add_one(void) {
    notif_store_init(&store);
    int rc = notif_store_add(&store, "s1", "my-project", "Waiting for input");
    assert(rc == 0);
    assert(notif_store_count(&store) == 1);
    const notification_t *n = notif_store_get(&store, 0);
    assert(n != NULL);
    assert(strcmp(n->id, "s1") == 0);
    assert(strcmp(n->project, "my-project") == 0);
    assert(strcmp(n->message, "Waiting for input") == 0);
    printf("  PASS: test_add_one\n");
}

static void test_dismiss(void) {
    notif_store_init(&store);
    notif_store_add(&store, "s1", "proj", "msg");
    notif_store_add(&store, "s2", "proj", "msg");
    assert(notif_store_count(&store) == 2);

    int rc = notif_store_dismiss(&store, "s1");
    assert(rc == 0);
    assert(notif_store_count(&store) == 1);

    // Dismiss nonexistent is idempotent
    rc = notif_store_dismiss(&store, "s1");
    assert(rc == -1);
    assert(notif_store_count(&store) == 1);
    printf("  PASS: test_dismiss\n");
}

static void test_clear(void) {
    notif_store_init(&store);
    notif_store_add(&store, "s1", "p", "m");
    notif_store_add(&store, "s2", "p", "m");
    notif_store_clear(&store);
    assert(notif_store_count(&store) == 0);
    printf("  PASS: test_clear\n");
}

static void test_update_existing(void) {
    notif_store_init(&store);
    notif_store_add(&store, "s1", "proj", "first");
    notif_store_add(&store, "s1", "proj", "updated");
    assert(notif_store_count(&store) == 1);
    const notification_t *n = notif_store_get(&store, 0);
    assert(strcmp(n->message, "updated") == 0);
    printf("  PASS: test_update_existing\n");
}

static void test_overflow_drops_oldest(void) {
    notif_store_init(&store);
    char id[8];
    for (int i = 0; i < NOTIF_MAX_COUNT; i++) {
        snprintf(id, sizeof(id), "s%d", i);
        notif_store_add(&store, id, "p", "m");
    }
    assert(notif_store_count(&store) == NOTIF_MAX_COUNT);

    // Add 9th — should drop "s0" (oldest by insertion order)
    notif_store_add(&store, "s_new", "p", "m");
    assert(notif_store_count(&store) == NOTIF_MAX_COUNT);

    // "s0" should be gone
    int found_s0 = 0;
    int found_new = 0;
    for (int i = 0; i < NOTIF_MAX_COUNT; i++) {
        const notification_t *n = notif_store_get(&store, i);
        if (n && strcmp(n->id, "s0") == 0) found_s0 = 1;
        if (n && strcmp(n->id, "s_new") == 0) found_new = 1;
    }
    assert(!found_s0);
    assert(found_new);
    printf("  PASS: test_overflow_drops_oldest\n");
}

// After dismiss+reuse, overflow must still evict by insertion order (seq),
// not by slot index. Regression test for the find_oldest bug.
static void test_overflow_after_dismiss_evicts_by_seq(void) {
    notif_store_init(&store);
    // Fill: s0(seq0)→slot0, s1(seq1)→slot1, ..., s7(seq7)→slot7
    char id[8];
    for (int i = 0; i < NOTIF_MAX_COUNT; i++) {
        snprintf(id, sizeof(id), "s%d", i);
        notif_store_add(&store, id, "p", "m");
    }

    // Dismiss s0 (slot0 freed), then add s8 — reuses slot0 with higher seq
    notif_store_dismiss(&store, "s0");
    notif_store_add(&store, "s8", "p", "m"); // s8(seq8)→slot0

    // Now full again. s1(seq1) is oldest by seq (in slot1).
    // Overflow should evict s1, NOT s8 (even though s8 is in slot0).
    notif_store_add(&store, "s9", "p", "m");
    assert(notif_store_count(&store) == NOTIF_MAX_COUNT);

    int found_s1 = 0, found_s8 = 0, found_s9 = 0;
    for (int i = 0; i < NOTIF_MAX_COUNT; i++) {
        const notification_t *n = notif_store_get(&store, i);
        if (n && strcmp(n->id, "s1") == 0) found_s1 = 1;
        if (n && strcmp(n->id, "s8") == 0) found_s8 = 1;
        if (n && strcmp(n->id, "s9") == 0) found_s9 = 1;
    }
    assert(!found_s1); // s1 was oldest by seq — evicted
    assert(found_s8);  // s8 is newer — kept
    assert(found_s9);  // s9 is newest — kept
    printf("  PASS: test_overflow_after_dismiss_evicts_by_seq\n");
}

// After filling the store, dismiss one slot, then add — should reuse the freed
// slot and NOT evict the oldest active entry.
static void test_full_dismiss_then_add_reuses_slot(void) {
    notif_store_init(&store);
    char id[8];
    for (int i = 0; i < NOTIF_MAX_COUNT; i++) {
        snprintf(id, sizeof(id), "s%d", i);
        notif_store_add(&store, id, "p", "m");
    }
    assert(notif_store_count(&store) == NOTIF_MAX_COUNT);

    // Dismiss the middle entry
    int rc = notif_store_dismiss(&store, "s3");
    assert(rc == 0);
    assert(notif_store_count(&store) == NOTIF_MAX_COUNT - 1);

    // Add a new one — should reuse freed slot, not evict s0
    notif_store_add(&store, "s_new", "p", "m");
    assert(notif_store_count(&store) == NOTIF_MAX_COUNT);

    int found_s0 = 0, found_s3 = 0, found_new = 0;
    for (int i = 0; i < NOTIF_MAX_COUNT; i++) {
        const notification_t *n = notif_store_get(&store, i);
        if (n && strcmp(n->id, "s0") == 0) found_s0 = 1;
        if (n && strcmp(n->id, "s3") == 0) found_s3 = 1;
        if (n && strcmp(n->id, "s_new") == 0) found_new = 1;
    }
    assert(found_s0);   // s0 must NOT have been evicted
    assert(!found_s3);  // s3 was dismissed
    assert(found_new);  // new entry present
    printf("  PASS: test_full_dismiss_then_add_reuses_slot\n");
}

// Strings at exact max length (MAX-1 usable chars) must be stored intact.
// Strings exceeding max length must be truncated with a valid null terminator.
static void test_max_length_strings(void) {
    notif_store_init(&store);

    // Build a string of exactly NOTIF_MAX_ID_LEN-1 = 47 chars (fits exactly)
    char long_id[NOTIF_MAX_ID_LEN + 4];
    memset(long_id, 'a', sizeof(long_id) - 1);
    long_id[sizeof(long_id) - 1] = '\0';

    // Exact-fit ID (47 chars)
    char exact_id[NOTIF_MAX_ID_LEN];
    memset(exact_id, 'a', NOTIF_MAX_ID_LEN - 1);
    exact_id[NOTIF_MAX_ID_LEN - 1] = '\0';

    int rc = notif_store_add(&store, exact_id, "p", "m");
    assert(rc == 0);
    const notification_t *n = notif_store_get(&store, 0);
    assert(n != NULL);
    assert(strlen(n->id) == NOTIF_MAX_ID_LEN - 1);
    assert(strcmp(n->id, exact_id) == 0);

    // Over-length ID (NOTIF_MAX_ID_LEN + 3 chars) — must be truncated
    notif_store_init(&store);
    char over_id[NOTIF_MAX_ID_LEN + 4];
    memset(over_id, 'b', sizeof(over_id) - 1);
    over_id[sizeof(over_id) - 1] = '\0';

    rc = notif_store_add(&store, over_id, "p", "m");
    assert(rc == 0);
    n = notif_store_get(&store, 0);
    assert(n != NULL);
    // Stored ID must be null-terminated and fit within the field
    assert(strlen(n->id) == NOTIF_MAX_ID_LEN - 1);
    assert(n->id[NOTIF_MAX_ID_LEN - 1] == '\0');

    // Over-length message — must be truncated and null-terminated
    notif_store_init(&store);
    char over_msg[NOTIF_MAX_MSG_LEN + 4];
    memset(over_msg, 'c', sizeof(over_msg) - 1);
    over_msg[sizeof(over_msg) - 1] = '\0';

    rc = notif_store_add(&store, "id1", "proj", over_msg);
    assert(rc == 0);
    n = notif_store_get(&store, 0);
    assert(n != NULL);
    assert(strlen(n->message) == NOTIF_MAX_MSG_LEN - 1);
    assert(n->message[NOTIF_MAX_MSG_LEN - 1] == '\0');

    printf("  PASS: test_max_length_strings\n");
}

// Empty string is a valid ID — it can be added, retrieved, and dismissed.
static void test_empty_string_id(void) {
    notif_store_init(&store);
    int rc = notif_store_add(&store, "", "proj", "msg");
    assert(rc == 0);
    assert(notif_store_count(&store) == 1);

    // Retrieve via get — slot 0 should be active
    const notification_t *n = notif_store_get(&store, 0);
    assert(n != NULL);
    assert(strcmp(n->id, "") == 0);

    // Update via empty-string ID
    rc = notif_store_add(&store, "", "proj", "updated");
    assert(rc == 0);
    assert(notif_store_count(&store) == 1);
    n = notif_store_get(&store, 0);
    assert(n != NULL);
    assert(strcmp(n->message, "updated") == 0);

    // Dismiss via empty-string ID
    rc = notif_store_dismiss(&store, "");
    assert(rc == 0);
    assert(notif_store_count(&store) == 0);

    printf("  PASS: test_empty_string_id\n");
}

// Count must stay consistent through repeated add/dismiss cycles.
static void test_count_consistency(void) {
    notif_store_init(&store);
    assert(notif_store_count(&store) == 0);

    notif_store_add(&store, "a", "p", "m");
    notif_store_add(&store, "b", "p", "m");
    notif_store_add(&store, "c", "p", "m");
    assert(notif_store_count(&store) == 3);

    notif_store_dismiss(&store, "b");
    assert(notif_store_count(&store) == 2);

    notif_store_add(&store, "d", "p", "m");
    assert(notif_store_count(&store) == 3);

    notif_store_dismiss(&store, "a");
    notif_store_dismiss(&store, "c");
    notif_store_dismiss(&store, "d");
    assert(notif_store_count(&store) == 0);

    // Re-add after drain — count restarts from 0 correctly
    notif_store_add(&store, "x", "p", "m");
    assert(notif_store_count(&store) == 1);

    printf("  PASS: test_count_consistency\n");
}

int main(void) {
    printf("Running notification store tests...\n");
    test_init_empty();
    test_add_one();
    test_dismiss();
    test_clear();
    test_update_existing();
    test_overflow_drops_oldest();
    test_overflow_after_dismiss_evicts_by_seq();
    test_full_dismiss_then_add_reuses_slot();
    test_max_length_strings();
    test_empty_string_id();
    test_count_consistency();
    printf("All tests passed!\n");
    return 0;
}
