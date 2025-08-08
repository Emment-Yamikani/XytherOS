/**
 * @file vfs_filename.h
 * @brief Virtual Filesystem Filename Utilities
 *
 * This module provides memory-safe utilities for allocating, creating,
 * duplicating, renaming, and comparing filenames within the VFS layer.
 * All filename structures are reference-counted.
 */

#pragma once

#include <core/types.h>
#include <sync/atomic.h>

/**
 * @struct vfs_filename_t
 * @brief Represents a managed, reference-counted filename in the VFS.
 *
 * @var name
 *      A null-terminated string containing the filename.
 * @var length
 *      Length of the name string, excluding the null terminator.
 * @var refcnt
 *      Reference count used for shared filename instances.
 */
typedef struct {
    char        *name;
    size_t      length;
    atomic_long refcnt;
} vfs_filename_t;

/**
 * @brief Creates a VFS filename from a raw string.
 *
 * Allocates and initializes a filename structure with a copy of the provided
 * null-terminated string. The reference count is initialized to 1.
 *
 * @param[in] name The raw filename string to copy.
 * @param[out] rfn On success, set to point to the new filename structure.
 * @return 0 on success, or a negative error code.
 */
extern int vfs_create_filename(const char *name, vfs_filename_t **rfn);

/**
 * @brief Releases a VFS filename structure.
 *
 * Decreases the reference count. If it reaches zero, frees the structure
 * and associated name string.
 *
 * @param[in] fn The filename to release. Safe to pass NULL.
 */
extern void vfs_destroy_filename(vfs_filename_t *fn);

/**
 * @brief Compares two VFS filenames for equality.
 *
 * Performs a byte-wise comparison of the name strings and lengths.
 *
 * @param[in] a First filename to compare.
 * @param[in] b Second filename to compare.
 * @return true if the names and lengths are equal, false otherwise.
 */
extern bool vfs_compare_filenames(const vfs_filename_t *a, const vfs_filename_t *b);

/**
 * @brief Compares a VFS filename to a raw string.
 *
 * Checks whether the internal name of the filename matches the provided
 * null-terminated string.
 *
 * @param[in] fn The VFS filename.
 * @param[in] raw The raw string to compare against.
 * @return true if the names are equal, false otherwise.
 */
extern bool vfs_compare_raw_filename(const vfs_filename_t *fn, const char *raw);

/**
 * @brief Replaces the contents of a VFS filename.
 *
 * Frees the current name string and replaces it with a copy of the
 * provided new name. Reference count is unchanged.
 *
 * @param[in,out] fn The filename structure to modify.
 * @param[in] new The new name string to assign.
 * @return 0 on success, or a negative error code.
 */
extern int vfs_rename_filename(vfs_filename_t *fn, const char *new);

/**
 * @brief Duplicates an existing filename structure.
 *
 * Allocates a new structure and copies the name and length.
 * The new instance has a reference count of 1.
 *
 * @param[in] src The filename to duplicate.
 * @param[out] rdst On success, set to the new duplicate.
 * @return 0 on success, or a negative error code.
 */
extern int vfs_clone_filename(const vfs_filename_t *src, vfs_filename_t **rdst);

extern void vfs_dump_filename(const vfs_filename_t *fn);

extern size_t vfs_filename_length(const vfs_filename_t *fn);