# Memory management review: Mem2d / ZData / TZData

Date: 2026-11-18
Files: `core-source/mem2d.cpp`, `core-source/mem2d.h`, `core-source/zdata.h`

This review checked the 2D/layered data storage classes (`Mem2d`, `ZData`, `TZData<ZTYP>`, `LayerInformation`) for memory leaks and related memory errors. The findings come from reading the code; no valgrind/ASan run has been done yet.

Status: **fixed** = changed in this revision; **noted** = reported, not changed.

---

## 1. Memory leaks

### 1.1 `TZData<ZTYP>::Resize()`: old data rows never freed (severe), **fixed**
In the general resize path (Nx or Nv changes, or Ny grows), the old row buffers were copied into the newly allocated rows, but only the row-pointer array (`delete [] Zdold`) was freed. The per-row `delete` was commented out as "obsolete" because it used the wrong (new) index `q`. Every such resize leaked nearly the whole old image: all rows `i < min(ny,Ny)`, plus the rows of layers `v >= Nv` when the layer count shrinks. `Mem2d::copy()` and `Mem2d::Resize()` both reach this path.

Fix: after copying, free all remaining old rows `Zdold[i*nv+v]` for `i < ny` (already reduced to `min(ny,Ny)`) and `v < nv`, then free the pointer array.

### 1.2 `Mem2d::L_info_new(keep=TRUE)`: inverted NULL checks, **fixed**
All three checks read `if (!layer_information_list)` and should have read `if (layer_information_list)`. When the number of layers changed with `keep_layer_info=TRUE`:
- the existing layer information was not carried over (the "keep" option had no effect),
- the `LayerInformation` objects and `GList`s of every old layer were leaked,
- the old `GList*[]` array was never freed.

Fix: corrected the three conditions.

### 1.3 `Mem2d::Mnew()`: histogram buffers leaked, **fixed**
`Init()` sets `Zbins = Zibins = NULL` and ran *before* `if(data) Mdelete();`, so `Mdelete()` could no longer free the histogram buffers allocated by `AutoHistogrammEvalMode()`. This leaked on every `Resize()` that changed the data type.

Fix: call `Mdelete()` before `Init()`.

### 1.4 `Mem2d::WriteScanEvents()`: temporary lists never freed, **fixed**
Each save allocated one local `Event` wrapper per probe event and one `UserEntry` copy per user-event set (each copy with its own `GPtrArray` and strings). Neither the objects nor the `GSList`s were freed; the code had "free ProbeEventsList and Elements!" placeholder comments where the cleanup belonged. The leak grew with every file save.

Fix: `g_slist_free_full()` both lists after use. For the probe list this deletes only the `Event` wrappers; the `ProbeEntry` objects stay owned by their `ScanEvent`s. The head of the sorted user list is now kept so the whole list can be freed.

### 1.5 `Mem2d::LoadScanEvents()`: user-event dims loop, **fixed**
Leak, and use-after-free / double free.
- `ue = new UserEntry(..., se, 0, this)` was created inside the loop but only deleted once after the loop. A `UserEntry` that loaded nothing (`se == NULL`) was overwritten on the next iteration and leaked.
- **Double free:** if the last matching dimension loaded successfully, that `UserEntry` was already attached to a `ScanEvent` (`se->add_event(this)` in the constructor). The `delete ue` after the loop freed it, and `RemoveScanEvents()` freed it again later.
- `message_id = g_strndup(...)` was never freed (the constructor `g_strdup`s its own copy).

Fix: inside the loop, delete `ue` only when `se == NULL` (not attached), always `g_free(message_id)`, and remove the `delete` after the loop.

---

## 2. Other memory errors

### 2.1 `ZData` constructor / `ZData::ZResize()`: `memset` byte count, **fixed**
`memset(Xlookup, 0, Nx)` cleared `Nx` bytes, not `Nx` doubles, so only 1/8 of each lookup table was zeroed (same for Y and V).
Fix: `Nx*sizeof(double)` etc.

### 2.2 `TZData<ZTYP>::Resize()` with `Zdat == NULL`: allocation at old size, **fixed**
`TNew()` was called before `ZResize()` updated `nx/ny/nv`, so the buffers were allocated with the *old* dimensions. Later accesses at the new size went out of bounds.
Fix: call `ZResize(Nx,Ny,Nv)` first, then `TNew()`, and return.

### 2.3 `TZData::Z_interpol2d()`, `order == 2` (GSL spline): out-of-bounds write, **fixed**
The grid loop runs `i,j = -1..2` and passed these directly to `gsl_spline2d_set()`, which takes `size_t`. Index -1 wraps around and writes outside `za`. The `malloc` result was also assigned without a cast (ill-formed C++).
Fix: use grid indices `i+1, j+1` (matching `xa = ya = {-1,0,1,2}`), add the cast, and remove the unused/shadowed `size_t i, j`.
Note: nothing currently calls this branch (all callers use the default `order=1`), and it still allocates a GSL spline for every pixel, which is slow.

### 2.4 `Mem2d(Mem2d *m, ...)` constructors: plane parameters, **fixed**
Both constructors called `GetPlaneMxyz(plane_mx, plane_my, plane_z0)` on *this*, which copies the object's own uninitialized members.
Fix: `m->GetPlaneMxyz(...)`.

### 2.5 `Mem2d` / `ZData`: implicit copy constructor and assignment, **fixed**
Both classes own raw buffers (`data`, `Li`, lookup tables, `Zdat`, layer info, scan events). An accidental copy by value would double free.
Fix: copy constructor and copy assignment declared `= delete`, so an accidental copy is now a compile error.

### 2.6 `MemDigiFilter::Convolve()`: threaded path, **fixed**
When `Dest->GetNy() < max_jobs`, some `job[]`/`tpi[]` entries were never initialized. The wait loop then read garbage `job` states and called `g_thread_join()` on garbage pointers.
Fix: limit `max_jobs` to `Dest->GetNy()`.

### 2.7 `MemDigiFilter::Convolve()`: single-CPU path discarded result, **fixed**
`sum` was computed but never stored.
Fix: `Dest->data->Z(sum, jj, ii)`, matching the threaded path.

---

## 3. Noted, not changed

- `TNew()` / `TZData::Resize()`: the `if(!Zdat[i])` checks after `new` never trigger, because `new` throws `std::bad_alloc` instead of returning NULL. They are harmless but give a false impression of out-of-memory handling.
- `MemDigiFilter::Convolve()` busy-waits (spins) on the job states while the threads run, and returns early on empty input without calling `progress_info_close()`.
- `UserEntry` (scan_event.cpp) uses `g_ptr_array_new()` without an element free function. Whether the `UE_set` elements are freed was not checked; it is outside this review.

## 4. Reviewed, no problems found
`ZData` constructor/destructor, `ZData::ResetLineInfo`, `TNew`/`TDel`, `Mem2d::Mdelete`, `Mem2d::L_info_delete`, `LayerInformation` (all constructors/destructor), `Mem2d::RemoveScanEvents`, the probe and old-style user-event loops in `LoadScanEvents`, `DataRead`, `DataD2DRead`, `ReportScanEvents*` (callers own the returned list copies).

## 5. Suggested verification
- Build and run a few typical sessions (multi-layer scan, resize/crop, save/load with events) under
  `valgrind --leak-check=full` or with `-Db_sanitize=address` (meson).
