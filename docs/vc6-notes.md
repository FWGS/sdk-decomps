# VC6 field notes: matching code from Visual C++ 6

These are the rules we learned about Visual C++ 6 (VC6) while rebuilding five
Half-Life mods: first function by function, then whole images byte for byte.
Counter-Strike, the sixth, taught more, about Visual C++ 5.0 and VC6's
service packs; [counter-strike.md](counter-strike.md) has those.
None of this is documented by Microsoft. It is written as a field guide for
someone matching their own VC6 binary: each rule comes with its evidence, an
example, and how to use it.

The compiler has three parts that matter here. **C1XX** is the C++ front end:
it parses the source and writes an intermediate language (IL); it also
decides in which order a file's inline functions are emitted. **C2** is the
back end: the inliner, the optimiser, register and stack allocation, the x87
code. **LINK** lays out the objects, drops unreferenced functions, folds
identical ones and writes the image. [reproducing.md](reproducing.md) covers
the parts of the image that are not code.

Most rules are for VC6 RTM (the first release, without service packs)
Professional, `C2.DLL` 12.00.8168, with `/O2`, which built Point of View,
Wanted!, Gunman Chronicles and They Hunger, and for LINK 6.00.8168. T2DEF's server was
built by the Standard edition, which ignores `/O2` and gives `/Od` code; its
client by Service Pack 4's Standard edition. Identify your compiler from the
Rich header before anything else ([compilers.md](compilers.md)).

Terms used below:

- **COMDAT**: a section of an object that holds one function or one datum,
  so the linker can drop it or keep one copy. Every inline function gets
  one; with `/Gy` (part of `/O2`) every function does.
- **Precompiled header** (`/YX`, the `.pch` file): a snapshot of the
  compiler's state after a file's first `#include`s, reused by the next file
  that starts with the same ones. Which headers are inside it changes where
  their inline functions are emitted.
- **Rich header**: the linker's undocumented record of how many objects each
  compiler build made ([reproducing.md](reproducing.md#the-rich-header-and-the-object-count)).
- **x87**: the floating-point unit, a stack of eight registers. Operand
  order shows as which value is loaded first (`fld`) and which is applied
  (`fadd`, `fmul`).
- **Near miss**: a function that matches retail except for a few
  instructions.

Commit ids are from the mod branches
([point-of-view](../../../tree/point-of-view), [t2def](../../../tree/t2def),
[wanted](../../../tree/wanted),
[gunman-chronicles](../../../tree/gunman-chronicles),
[they-hunger](../../../tree/they-hunger)). `git show <id>` shows
the change; most commit messages hold the sweep of variants behind it.

## How sure each rule is

Each rule carries one of three marks:

- **Exact**: read from the compiler's or linker's code, or a black-box rule
  that predicted every test made with it, including predictions written down
  before compiling.
- **Model**: predicts what we tested, but the mechanism is a guess, or it has
  known gaps.
- **Observed**: seen in one or a few functions. A lever worth trying, not a
  law.

## Contents

- [Before you start](#before-you-start)
- [The inliner's budget](#the-inliners-budget)
- [Stack slots](#stack-slots)
- [Register choice](#register-choice)
- [x87 operand order](#x87-operand-order)
- [Member reads through a local pointer](#member-reads-through-a-local-pointer)
- [Tail merging and block layout](#tail-merging-and-block-layout)
- [Text that compiles to nothing still counts](#text-that-compiles-to-nothing-still-counts)
- [Where header inline functions land](#where-header-inline-functions-land)
- [Section order and COMDAT placement](#section-order-and-comdat-placement)
- [LINK's identical-code folding](#links-identical-code-folding)
- [`.bss` order](#bss-order)
- [What leaves no trace](#what-leaves-no-trace)
- [A working order](#a-working-order)

## Before you start

- **Build the reference from the SDK first.** Each mod started from a VC6
  build of the Half-Life SDK version it was based on. The SDK build that
  matched most bytes picked the base, and then the mod's changes were
  written function by function.
- **A matcher that masks addresses lies sometimes.** Comparing functions
  with relocated operands masked is the fast way to find matches, but it
  calls two functions equal when one calls `Vector::Length` and the other
  `Length2D` (two identical-shaped inlines; T2DEF had this wrong in several
  monsters' functions for a while), or when one loads another float
  constant. Before calling a function done, compare its call targets and
  constants too.
- **Full builds only.** An incremental build can move header inlines
  between objects (see
  [Where header inline functions land](#where-header-inline-functions-land)).
  Numbers from incremental builds are not numbers.
- **Expect a few long functions to settle last.** The set of symbols in the
  shared headers moves single instructions in long functions: an unused
  six-entry enum in Wanted's `nodes.cpp` flipped `BuildNodeGraph` between
  exact and one instruction off; prototypes added to `util.h` changed
  register choice in Point of View's `nodes.obj`. Finish the headers before
  chasing those. The rule behind it is under
  [Reloads at a loop end](#reloads-at-a-loop-end-symbol-numbers).
- **Compiler switches that help:**
  - `/Bk<base>` keeps C1XX's IL files (`<base>gl`, `sy`, `ex`, `in`). The
    `ex` file gives each function's IL offset, which the emission rules
    below use.
  - `/Bd` prints the command line of each compiler pass.
  - `#pragma warning( 1: 4710 )` in a file passed with `/FI` makes C2 print
    warning C4710 for every inline candidate it rejects, in decision order,
    with the call site.
  - `#pragma inline_depth( 0 )` the same way makes C4710 list every
    candidate site of every function, in the order the inliner sees them.
- **Test files are cheap.** Most rules here came from copying one function
  into a small file, generating hundreds of variants, compiling them in
  parallel and comparing the code. Patched copies of `C2.DLL` (for example
  with a larger memory arena) showed what does *not* matter.

## The inliner's budget

**Exact** (decoded from `C2.DLL`'s code; predictions made before compiling
came out exactly in 62 of 62, 13 of 13, 16 of 16 and 9 of 9 cases, and 271
of 271 decisions in one 51-function file; Wanted).

With `/O2` (which includes `/Ob1`), C2 decides for each function which calls
to inline functions it expands:

1. **The budget.** S is the size of the calling function as C1XX counted it
   (it behaves like a count of IL nodes before optimisation; the costs below
   are measured). The budget is 2 × S, at least
   1000 and at most 35000. So S only matters above 500.
2. **The sites.** C2 lists the function's calls to inline functions in
   evaluation order: n sites. Calls to ordinary functions don't count.
3. **The decisions,** in list order. Let s be the callee's size.
   - Callees of size 40 or less are free: never checked, never charged
     (what their own calls spend is charged). But they count in n.
   - A bigger callee is rejected if s is more than what is left. A rejection
     costs nothing, so a smaller callee later can still fit.
   - An accepted callee is charged s. Then its own sites are decided at
     once, depth first, with a share of what is left: the remaining budget
     divided by the number of sites still to come, this one included
     (integer division). What they spend comes off the main budget.
4. **By-value class parameters** (empirical): a function with a class
   parameter passed by value is never inlined, and no inline call inside the
   argument list of a call with a by-value class parameter is inlined.
5. **Depth.** Each site carries the `#pragma inline_depth` in effect where its
   code was compiled; for calls inside an inline function, where that
   function was defined. The default is 8.

Sizes of the SDK's `vector.h` members: `Vector( float, float, float )` 47,
copy constructor 57, `operator+` and `operator-` 76, `operator*` and
`operator/` 67, `Length` 59, `Length2D` 47, `Normalize` 119, unary minus 63,
`DotProduct` 51, `float * Vector` 43, `CrossProduct` 108. Free ones:
`Vector()` 20, `operator float *()` 22, `ENT()` 20 or 23, `EMIT_SOUND` 40.
Other SDK inlines: `TraceResult()` 42, `FClassnameIs` 36, the egon's
`HasAmmo()` 46 and `UseAmmo( int )` 84.

What a statement adds to S (measured): `int i;` 0, `int i = 1;` 4, `x = 1;`
7, `x = y;` 11, `Vector v;` 11 (and one site), `Vector v = a;` 12,
`Vector v( a );` 16, `Vector v; v = a;` 19, `Vector v = a + b;` 25,
`Vector v; v = a + b;` 32, `f = a.x;` 13, `f = a[0];` 18, a hand-written dot
product 53, `DotProduct( a, b )` 19.

**What it means in practice:**

- **Retail's tell.** An operator inlined while its own
  `Vector( float, float, float )` or return copy is called out of line means
  that site's share ran out. Count the sites before it.
- **The budget runs backwards.** Early sites get thin shares, so the first
  statements lose their nested calls first, and the last sites get the
  remainder. Code added *after* a spot can take away its inlining (more
  sites, thinner shares). Among identical inline copies, the last in source
  order gets the most.
- **Dead code counts.** S is the function as written: dead stores, unused
  initialised locals, `if ( 0 )` blocks with statements in them, statements
  after a `return`, discarded results (`vec.Normalize();`). The optimiser
  removes them later; the budget was set before. Uninitialised declarations
  cost nothing. A function below S = 500 gets 1000 whatever it contains,
  which is one reason the same dead store helps in one function and does
  nothing in another.
- **Free sites thin the shares.** Every `v[i]` on a `Vector` is a call to the
  free `operator float *()`; so are `ENT()` and `EMIT_SOUND`. They cost no
  budget but split it further. A subscript is a natural way to add a site
  where the shares need one: Wanted's `CBot::Wander` writes `vecCell[2] += 1`
  (`ab51ea6`); five of Gunman's homing rockets use subscripts (`7617548`).
- **Inline functions move IL into a nested list.** A small inline function
  that wraps a few statements gets one share and spends it on its own sites,
  instead of each site getting a share of the caller's list. Wanted's
  `CBot::MoveToLead` is impossible as one function body under this rule; it
  compiles to retail's code with its last line in an inline member
  (`HeadFor`, name ours). The bots' source evidently used such functions
  everywhere. `BotThink`'s statements up to `if ( IsAlive() )` are an inline
  `BeginFrame` of size 959, which leaves 41 of the 1000: every callee above
  40 is then called out of line, which is retail's pattern.
- **Spelling changes size and sites.** `Vector v = a + b;` adds a copy
  constructor site that `Vector v; v = a + b;` doesn't; `DotProduct()` costs
  more than a hand-written dot product; `EMIT_SOUND` is an inline wrapper
  around `EMIT_SOUND_DYN`, which is a plain call.
- **`#pragma inline_depth( 1 )`** before a group of inline functions expands
  them but none of their own calls. Used once, marked `// sic`: Wanted's
  `CBotMemory::Update` (`17f79ea`).
- **The budget can pull against another rule.** In They Hunger's
  `CEgon::PrimaryAttack` (`a2276df`) retail keeps the ammo count in a
  register from its test to its decrement, which VC6 does only when both go
  through one pointer or reference variable
  ([below](#member-reads-through-a-local-pointer)). But a pointer replaces
  the SDK's `HasAmmo()` and `UseAmmo( 1 )`, which cost 46 + 84 = 130. The
  function's size is 487, so its budget is the minimum 1000, and with 130
  of it unspent VC6 expands `Vector( x, y, z )` in the function's last two
  sums, which retail calls. Retail's tail needs 102 to 177 units spent
  before those sums (55 to 130 if one more inline call follows them).
  Unused locals with inline constructors, and one `HasAmmo()` after the
  sums, spend them. Many such forms compile to retail's bytes and none is
  forced, so the lines are marked `// sic`.
- **Sweeping.** When nothing visible explains retail's inlining, add N dead
  stores and try N = 0, 1, 2, ... Gunman's
  `CBasePlayer::CheatImpulseCommands` needs exactly seven (`2ffd851`),
  `CLightningBug::TrackTargetBug` exactly five (`ca93bf1`). Prefer real
  leftovers from the SDK text the mod copied (an unused
  `TraceResult tr, beam_tr;`, the apache's unused `vecEst`) over invented
  lines, and mark invented ones `// sic`.

An earlier working model (level by level, greedy by size) fit many cases
before the decoded rule replaced it. Front ends differ too: Service Pack 4's
Standard edition with `/O2` expands inline functions that T2DEF's client
keeps out of line (the build adds `/Ob0`; whether T2DEF's project did is
open), and its front end builds by-value temporaries in place, which RTM's
never does.

## Stack slots

### Optimised code: the stack packer

**Exact** (read from C2's code and checked with an instrumented copy of the
compiler; Wanted `15d6a91`).

1. **References.** Each stack symbol gets +1 for every operand naming it in
   the low-level IL after register allocation, but before tail merging,
   hoisting of common first instructions and the `fstp m; fld m` → `fst m`
   peephole. References those passes remove still count.
2. **Order.** Symbols sorted by size ascending, then by reference count
   descending (ties: the one that reached its count first).
3. **Packing.** Each local joins the most recent compatible slot (its size
   at most twice the slot's, no overlapping lifetime; counts add) or opens a
   new one. Parameters come first, and locals may share the incoming argument
   slots; if the frame is then over 0x6f bytes, packing is redone without
   that.
4. **Large frames.** Over 128 bytes, slots are sorted by density
   (count × 1000 / size) with a quicksort that is not stable.
5. Slot 0 is nearest `esp`.

**Lever:** to add references without changing code, write a statement in
both branches of an `if`; VC6 merges the copies afterwards, but the
references were counted. Wanted's `AvoidWalls` turns `vecAngles.y` in each
dodge branch: 17 references become 19, the 12-byte vector's density goes
from 1416 to 1583, past 1500, and the frame takes retail's order with the
same code.

Rules seen before the packer was read, all consistent with it
(**Observed**):

- Temporaries of one full expression never share slots. A frame several
  `Vector`s too large means separate statements in your source were one
  expression in retail's.
- A block's temporaries share slots with that block's locals, not with an
  enclosing scope's. A frame 12 bytes too small usually means a `Vector`
  local belongs one scope further out.
- A named `Vector` initialised from a returned `Vector` packs into the return
  temporary (copy source and destination don't overlap).
- Taking a local's address anywhere, even in dead code, stops temporaries
  from sharing its slot.
- Locals are numbered by their first IL reference, dead initialised ones
  included, and that numbering breaks ties (Gunman's customization menu
  constructor, `bb0c6fe`).

### Unoptimised code (`/Od`): the slot hash

**Exact** (checked against about 480 names on the VC6 objects of Sweet
Half-Life, another mod; used for T2DEF's server).

In `/Od` code the slots of one scope follow the locals' names, not their
order. The hash is the same as the [`.bss` hash](#bss-order) with 16 buckets:
`h = h*4 + (h >> 4) + c` over the name, bucket `(h ^ (h >> 16)) & 15`. Slots
go down from `ebp-4`, bucket 0 first; inside a bucket the later-declared
local comes first. Inner scopes come after their enclosing scope, in source
order; sibling scopes (`if`/`else`) don't share slots. So every local whose
name you made up must be named to fit retail's frame (T2DEF `06e60f7`,
`318af52`).

## Register choice

### Integer registers: priorities

**Exact** (read from C2's colouring pass; Wanted `3ae2513`).

C2 colours candidates greedily, highest priority P first. For each basic
block b, with w = 2^(loop depth) and N = the number of distinct
register-class symbols referenced in b:

- a candidate referenced in b gains N · w · (the sum of its reference costs:
  mostly 2; 1 for a constant temporary, whose definition counts −1 · w);
- a candidate live through b without a reference loses N · w.

Black-box probes flip exactly at 2k+1, 4k+2 and 8k+4 uses for loop depth 1,
2 and 3. Loop depth comes from the flow graph, so a loop made with `goto`
counts. Dead stores are removed before the weighting, so they can't steer
it.

**Lever:** a loop-carried flag. Wanted's `BotFindProfile` loops on
`while ( fMore )` with `fMore = FindNextFile( ... )`: the flag is threaded
away afterwards, but it lifts the find handle's priority above a hoisted
constant's, and the constant loses `ebp` as in retail.

### `eax` or `edx`: entity words

**Model** (Wanted `361a0b0`).

Which of two values gets `eax` and which `edx` can follow the same bit
vector of entity numbers as the [x87 order](#x87-operand-order): whether
the two entities sit in the same 32-bit word or not. In Wanted's
`CScorpion::HuntThink`, `gpGlobals` is numbered at its first use and the
`TraceResult` at its declaration, 6 apart in one word in ours. Live int
locals inserted before `gpGlobals`' first use gave retail's registers for
25 to 30 and 57 to 62 of them (period 32). The natural fix: the block
declares its own `TraceResult tr;`. Entities measured: an unused initialised
int is one, an unused `Vector` or `TraceResult` two, an uninitialised int
none.

### The vtable register: least recent use

**Observed** (Gunman `501bcb0`).

For consecutive blocks that each make a virtual call, VC6 picks the register
for the vtable by least recent use of `eax` and `edx`. Three calls alone
alternate `edx`, `eax`, `edx`, and can't be merged. In retail's
`CBeamGun::WeaponIdle` each call is followed by a store that uses `eax`, so
all three take `edx`, and cross-jumping merges them into one shared call:
retail's code comes from one call per `case`, each followed by the store.

### Registers are chosen before tails merge

**Observed** in several functions. VC6 allocates registers first and merges
identical tails afterwards. A case body or tail that merges away still moves
the register choice of everything after it. Gunman's
`CBaseBandit::GetScheduleOfType` needs the SDK's case order, with an
`ELOF_FAIL` copy that merges away (`e992bea`); Wanted's `StartRound` ends
both branches with their own copy of the same two statements (`f9d9a24`).

### Reloads at a loop end: symbol numbers

**Exact** as a black-box rule (They Hunger `0ffe1b9`, `2b0dedb`); why C2
orders the reloads this way was not read from its code.

In a few long functions the order in which C2 reloads two or three
registers at one point (the end of a loop, in the two `nodes.cpp` functions
below) is moved by nothing inside the function. The order follows the
numbers C1XX gives the symbols of the file.

- **C1XX numbers every symbol a file declares, in order,** headers
  included. C2 gets only the numbers: the IL of two compiles that differ by
  one unused declaration has the same size and differs only in number
  fields, each by one. The unused declaration itself is not written.
- **Only the function's compiler-made symbols matter:** its labels and
  temporaries, not its parameters or named locals. N unused symbols give
  the same code whether they are declared before all headers, after the
  includes, right before the function, at the start of its body or after
  its named locals. Declared after the function they do nothing. So what
  counts is how many symbols are declared before the function's body,
  anywhere.
- **Modulo 64** (checked for 0 to 70 extra symbols).
- **What a declaration costs,** measured as the shift of a later local's
  number:

  | Declaration | Symbols |
  |---|---|
  | a variable (`extern`, `static` or `const` too), a `typedef`, a forward declaration of a class | 1 |
  | a repeated `extern` of the same variable | 0 |
  | a prototype | 1 + its parameters, at least 2; a repeated prototype counts again |
  | an enum | 1 + its constants |
  | a struct or class | 8, + 1 per data member, + 2 per member function (5 for a virtual one) |
  | a function definition | 3, and more for its parameters, locals, string literals and labels |
  | each distinct included file | 1; a guarded header included again 0 |
  | `#define`, `#pragma`, comments | 0 |

  `/Zi` and `/YX` change nothing.
- **One function, one mapping.** `CGraph::LinkVisibleNodes` (SDK
  `nodes.cpp`) has three reloads at a loop end that go through five
  arrangements per period of 64. The mapping from count to arrangement is
  the same in the Point of View, Gunman Chronicles, Wanted! and They Hunger
  trees, so it belongs to the function, not to a tree. In They Hunger,
  retail's arrangement needs 19 to 54 more symbols than the tree's headers
  declare. `CTestHull::BuildNodeGraph`, in the same file, follows the same
  count and allows 0, 5 to 16, 20 to 24, 34 to 47 and 56 to 63, so both are
  retail's for 20 to 24 and 34 to 47.
- **Few functions follow the count at all.** We compiled every source of
  They Hunger's two projects with 0 to 63 extra symbols in front (6,400 and
  3,400 compiles). In the server only `LinkVisibleNodes` and
  `BuildNodeGraph` change, in the client only the constructors of
  `CClassMenuPanel` and `CMessageWindowPanel`.
- **Where the symbols are declared matters a little** when inline functions
  with compiler-made symbols of their own come between them and the
  function. For `CClassMenuPanel`'s constructor, 5 to 39 or 46 to 59 extra
  symbols after the file's includes give retail's order and leave the other
  constructor alone; at the end of `hud.h`, which
  `vgui_TeamFortressViewport.h` with its inline constructors follows, only
  5 to 7, 10 to 13, 16, 21 to 39, 49, 51 to 54 and 57 to 59 do.

**How to use it.** When a long function differs from retail only in the
order of reloads at a loop end, and no text inside it moves that:

1. Sweep an unused enum of N constants after the file's includes, N = 0 to
   63, and note which N give retail's code. Do the same for every other
   function of the file that follows the count; the windows must overlap.
2. Look for real declarations of that count: another SDK version of a
   header, a prototype or class the mod would have added. Count them with
   the table.
3. A fit alone is weak evidence: about one count in six fitted all four of
   They Hunger's functions by chance, and two different explanations both
   fitted (an engine header without one unused block; the mod's weapon ids
   as an enum in a shared header) and excluded each other. If nothing else
   in the image decides, use a marked unused enum where no other file sees
   it, and say so. Wanted's `nodes.cpp` has one for the same two functions
   (`d19fb1d`).

### Other levers

All **Observed**:

- One extra used local, even one the optimiser propagates as a constant,
  changes register choice.
- The number of `return`s moves the whole allocation: one shared final
  `return TRUE` instead of one per branch gave Wanted's
  `CWantedMultiplay::ClientCommand` retail's registers.
- `T( void ) { }` and `T() { }` compile differently. The spelling of an empty
  inline constructor changes register allocation where it is inlined, and the
  COMDAT order of every object that includes the header (Point of View
  `a1f3097`, five classes in `vgui_TeamFortressViewport.h`).
- Two stores of constant 0 make VC6 hold a zero register over the code
  between them.
- An inline accessor that takes a reference moves registers without changing
  code (Wanted `7088e3e`).
- A shared address constant: written `(float *)&g_vecZero` twice, VC6 loads
  one constant for both; spelling one as `(float *)&g_vecZero[0]` gave
  Gunman's `CEndBossKataball::KataTouch` retail's `edi`/`ebp` (`ec294f4`,
  after 512 combinations of no-code changes moved nothing).
- In the functions studied, enum constants, unused locals, discarded
  expressions, local names and a larger memory arena for C2 never moved an
  allocation, except in the few long functions that follow the count of
  declared symbols ([above](#reloads-at-a-loop-end-symbol-numbers)).

### x87 registers

**Exact** in its main lines (read from C2; Wanted `455d243`). x87 values are
allocated by a pass of their own, before the integer registers: a backward
walk over each block that tracks stack depth and last uses, priorities per
reference weighted by 2^(loop depth), and a value is kept on the FPU stack
only if its definition and death nest last-in-first-out with the values
already kept. A loser is spilled to its memory home. Wanted's `CheckAhead`
gets retail's spill from `?:` inside the `Vector` arguments, which makes each
wrapped value a new value.

## x87 operand order

### The bit-vector walk

**Exact** as a black-box rule; where in C2 it happens was not found. Tested
on about 2,700 small test functions and about 250 variants of one file
(Gunman `d4b4311`).

For a commutative x87 operation, and for the terms of a sum C2 reassociates
(such as an inlined `Length()`), C2 orders the operands by walking a bit
vector of the entities it tracks (variables and temporaries; each field of a
struct local is one) one 32-bit word at a time: words upwards, bits
downwards inside a word. So inside one word, the entity with the higher
number comes first.

Fields that a function reaches only through inlined members (copy
constructor, `Normalize`, `Length`) are numbered late and next to each
other, x, y, z. So they come out z, y, x, unless a word boundary falls
between them:

| Where the 32-entity word boundary falls | Order |
|---|---|
| not between x, y and z | z, y, x |
| between y and z | y, x, z |
| between x and y | x, z, y |

- A component the function names directly (`v.x` in the source, not `v[0]`
  and not an inline member's `this->x`) becomes a variable of its own. It
  ranks below `Normalize`'s `flLen`, in reverse naming order, wherever the
  reference sits, even in dead code. The inlined code that reads it changes
  form: `fld st(0); fmul [x]` against `fld [x]; fmul st(1)` in `Normalize`,
  and the term order in `Length` and dot products.
- Each live int local declared before the vector moves the boundary by one,
  with period 32 (y, x, z at 76, 108, 172 and 204 extra locals in one test).
  Uninitialised locals and dead stores don't count.
- Addresses play no part: a copy of `C2.DLL` patched to a 256 KB memory
  block, and either C runtime, change nothing.
- A likely mechanism (**Model**): C2 allocates its symbol records 32 to a
  memory block, each with a fixed entity number, so one block is one word of
  the bit vector; freed records go on per-kind lists and keep their numbers.

**How to use it.** Read the order of retail's terms in an inlined
`Length()` or of the multiplies in `Normalize()`, then:

- To push a component below the others, name it. A no-op like
  `vecVelocity.z = vecVelocity.z * 1;` does it (Gunman `8ca0db5`, `22ca208`,
  `c30852c`).
- To move an unnamed component into an earlier word without naming it, read
  it through a small inline function that takes `const Vector &`. Gunman's
  `CBeak::StartTask` needs y ahead of z, both unnamed (`d4b4311`):

  ```cpp
  // sic: StartTask reads vecEnemyVelocity.y through this inline function
  inline float VecY( const Vector &vec ) { return vec.y; }
  ...
  Vector vecEnemyVelocity = m_hEnemy->pev->velocity;
  float flEnemyY = VecY( vecEnemyVelocity );	// sic: unused
  ```

  That numbers y in the word before z's, which gives retail's y, z, x in the
  inlined `Length()` and z ahead of `flLen` in `Normalize()`. Nothing else in
  the function moves. A direct read names y; `operator[]`, `Make2D`,
  `DotProduct`, `CrossProduct` and `Vector` copies either name y or change the
  inlining.
- Moving the boundary with extra live locals works in test files, but in a
  real function the extra locals move the inliner or other orders first.

### Other operand-order rules

All **Observed** unless marked:

- An inlined `Vector::operator+` loads its right operand first:
  retail's `fld punchangle; fadd v_angle` is `v_angle + punchangle`. A member
  operator evaluates its argument first.
- Comparisons keep the source's order: `time >= m_flX` loads `time`,
  `m_flX <= time` loads `m_flX`. `this != pEntity` and `pEntity != this` swap
  the `cmp` operands.
- For plain float arithmetic the written order of commutative operands
  doesn't matter (216 combinations gave one code).
- `.x` against `[i]` on one operand flips an x87 add (the subscript is an
  inline call).
- `Length2D()` and `Make2D().Length()` load x and y in different orders.
- An inline float accessor is evaluated first: `x - obj.Get()` loads the
  getter's value first (`fld [m]; fsubr x`), `x - obj.m` loads `x` first.
- A dead store anywhere in a function can flip a commutative x87 add
  elsewhere in it (Point of View `7c45126`, `7504140`). In Wanted's
  `CheatImpulseCommands` the count of stored locals modulo 8 picked the
  order.
- **Parentheses are tree nodes.** VC6 keeps redundant parentheses in the
  expression tree. `x = ( a * k );` keeps the x87 store right after the
  `fmul`; without them the store sinks past the next statement's argument
  pushes (Point of View `396fe00`, `d7a3267`). When a near miss differs only
  in where one x87 store sits against pushes, try parentheses around the
  whole stored expression or argument. Redundant parentheses around an
  inlined call change its add order; in one basic block they also move the
  scheduling of later independent instructions; enough of them (32 pairs in
  one test) change the inlining.
- **Constants.** An int literal in float arithmetic becomes a float constant
  (`+ 4` loads a dword, `+ 4.0` a qword). `x / 2` becomes `x * 0.5f`, `x * 2`
  becomes `fadd st, st`. A division by a constant becomes a multiply by a
  constant named after its extended value, so `avelocity / 10` and
  `avelocity * 0.1` load two different constants (Gunman `0dc77a5`).
- **Not solved:** whether an inline function's parameter counts as dying at
  its last multiply (`fmul m`, or `fld m; fmul st(1)` and a later
  `fstp st(0)`). Earlier inline expansions in the function set it with fixed
  periods. This is the 4 bytes left in Wanted's `CBot::Wander`.

## Member reads through a local pointer

**Exact** for the forms tested (40 read-pair variants in a test file;
Gunman `b68bb86`).

VC6 merges two reads of the same member into one load in every form tried,
including through inline functions. It does not merge a member read with a
read through a local pointer or reference that holds the member's address:
copy propagation folds the address back into `[esi+offset]` only after
common subexpressions have been merged. So when retail loads a member twice
in a row, write one of the reads through a pointer. Gunman's
`CBaseTurn::TurnTo`:

```cpp
int *piAngle;	// sic
...
piAngle = &m_iYaw;
iCurrent = *piAngle;                    // mov edi, [esi+m_iYaw]
flCurrent = m_iYaw + pev->angles.y;     // fild [esi+m_iYaw]: a second load
```

Only `int *`, `int &`, `const int &`, a pointer to member, and an inline
function returning a reference through `?:` kept the loads apart. An inline
function taking the member by reference doesn't: the inliner substitutes the
argument. The rule doesn't apply to locals: pointers and references to a
local fold away before the merge.

The other direction (**Observed**: one function, many variants; They
Hunger `a2276df`). When a test and a later decrement of the same count both
go through one pointer or reference variable, VC6 takes the address first,
loads the count once with the address folded into the load, and keeps it in
a register in between, even across a test that reads other memory:

```cpp
int &iAmmo = m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType];	// sic
if ( iAmmo > 0 && m_pPlayer->pev->waterlevel != 3 )
{
	if ( iAmmo >= 1 )
		iAmmo -= 1;
```

With the member expression itself, as the SDK's inline `HasAmmo()` and
`UseAmmo( 1 )` read it, VC6 loads the count for the first test, makes an
address temporary and loads again through it after the second test. It
keeps the count only when nothing between the two tests reads memory, and
then the code differs elsewhere. Unused locals and a uniform shift of the
symbol numbers never change this.

## Tail merging and block layout

All **Observed**, each in one or more retail functions.

- **Tails are merged as statements, before scheduling and after register
  choice.** A statement after an `if`/`else` is copied into both arms with
  the same registers. When retail shows that, write the statement once,
  after the `if`/`else`. Conversely, a statement written in both arms
  becomes one shared tail with other registers.
- **Identical exits.** Identical `{ A; return; }` blocks are merged. When two
  identical sequences end a function, VC6 keeps the copy that ends with an
  explicit `return` and jumps to it from the other. When retail jumps to the
  same code from two tests, the source had two such blocks, not an `else`.
- **Separate `if`s against an `if`/`else if` chain.** The compares are the
  same, but VC6 keeps a different copy of a shared tail. Wanted's
  `SpawnRandomGibs` uses five independent `if`s.
- **A flag for a shared tail.** Setting a `BOOL` in each branch and doing the
  tail once under `if ( flag )` gives other registers than letting VC6 merge
  two copies; the flag is threaded away.
- **Calls with different constants** in two arms are not merged; computing
  the value into a local and making one call is.
- **Identical arms leave a dead compare.** VC6 drops the jump but keeps the
  `cmp`: retail's dead compares mean both arms were written out.
- **Switches.** A switch compiled as a compare chain places the case bodies in
  reverse source order (fall-throughs keep theirs). After a jump-table
  switch, the code that follows is placed right after the first case body
  (in source order) that falls into it, and a statement right after the
  switch is copied into the end of every case. Identical case bodies that
  are not adjacent merge only in some case orders.
- **Loops.** A loop whose index is otherwise unused becomes a count-down; a
  `while ( 1 )` left by `break` keeps counting up. A jump into a loop, even
  `if ( 1 ) goto label;` to a label right after `do {`, makes it an improper
  loop: constant propagation stops through it and the loops around it are
  not rotated. Gunman's `CHudAmmo::DrawWList` needs one (`84d8397`, marked
  `// sic`).

## Text that compiles to nothing still counts

Statements the optimiser deletes, comments and blank lines, unused inline
functions: none of them leaves an instruction, but some act through two
dials.

1. **The inliner's size S** ([above](#the-inliners-budget)): dead
   statements count; declarations without initialisers don't.
2. **The IL offsets** that place header inline functions
   ([next section](#where-header-inline-functions-land)). Every byte of IL
   before a function moves the point where VC6 emits deferred inlines.

Neither reaches the scheduler: dead code leaves nothing in the instruction
stream, so it can't move where a call's set-up lands inside a long basic
block. That is why two of Gunman's client functions (`DrawShocks`,
`DrawSprites`) stay a few lines off.

IL costs, measured with `/Bk` (**Exact** for the objects measured; the
numbers from different files disagree in detail, so measure your own):

| Text | IL bytes |
|---|---|
| a line where a statement starts: an empty `;`, an unused declaration | 5 |
| a label | 8 |
| `{ }` | 13 |
| braces around a single statement (`if ( a ) { b; }` against `if ( a ) b;`) | 18 (They Hunger's `ammo.cpp`) |
| a line record | 3 below line 128, 5 from line 128 on (Wanted's `aflock.cpp`); Point of View measured 1 byte more per line from 128 |
| an `#include` | 7 on entering and 7 on returning (9 each from line 128), also when the include guard skips the header's body (Wanted, Gunman); one later measurement found repeated guarded includes free |
| a blank line, a comment, a prototype, a `;` at file scope | 0 by itself |
| an unused inline function in a header | its IL (only used ones get a symbol). 60 empty statements on 60 lines: 366 (They Hunger's `weapons.h`) |
| an unused inline function in a `.cpp` | its IL and nothing else: no code, no data, no relocation, no symbol. 44 statements of the form `p[0] = p[1] * 2 + p[2];`: 3,149 (They Hunger's `hl_weapons.cpp`) |
| a local's name | 0 |
| a new distinct class type among the members of a class the file uses | about 31 lines' worth (Gunman's `CHud`) |

Blank lines and comments act only through line numbers: they shift every
later line, and a statement on line 128 or later costs more than one below.
So blank lines near the top of a file can move the emission order; blank
lines after line 128 can't. Point of View's `ammo.cpp` showed it: blank
lines inserted in lines 1 to 92 moved the order in steps; inserted after
line 140, nothing.

Examples, each marked `// sic` in the source:

- Point of View `469339a`: nine empty statements at the top of
  `CHudAmmo::Draw`. Release builds with 8, 9 or 10 give retail's file; 7 and
  11 don't. They stand in for text retail had in `Draw` or after it.
- T2DEF `85bcd19`: braces on single-statement bodies in `aflock.cpp`'s
  `FlockLeaderThink` (any three of the five work).
- Wanted `c5175b2`: ten comment lines in `util.h` above its inline
  functions, for the 8 to 12 lines retail's `util.h` had there.
- Wanted `ca08901`: an unused 42-statement inline function in `bot.cpp`
  standing in for about 2,250 bytes of IL that compiled to no code.
- They Hunger `4040aea`: eleven brace pairs around single statements taken
  out of `ammo.cpp` (198 bytes), because retail's text had 172 to 206 bytes
  less by the end of `GetSpriteList`. Ten pairs fit as well. This one is
  not marked: unbraced statements are the SDK's own style there.
- They Hunger `0aab512`: an unused inline function of 60 empty statements
  in `weapons.h`, for the 325 to 403 bytes of inline IL retail's header had
  in front of one class (52 to 67 statements work).
- They Hunger `e553179`: an unused 44-statement inline function in the
  client's `hl_weapons.cpp`, for 3,095 to 3,290 bytes of IL (44 to 46
  statements work).

Retail had *some* text there. Uncalled inline functions, `if ( 0 )` blocks
and statements after a `return` all add IL, leave every function's code the
same, and leave no string or constant in the object. Which one it was can't
be read from the image.

How much text it was can be measured, and the rest of the image narrows
what it was. For They Hunger's `hl_weapons.cpp`: a sweep of empty
statements gave the size (619 to 658 lines of 5 bytes fit, 618 and 659
don't); the client's `.reloc` size said the text had no absolute address
([reproducing.md](reproducing.md#the-reloc-size)); LINK's fold sort said it
added no function; `.bss` said it added no static.

## Where header inline functions land

**Exact** (predicted 12 of 12 compiled variants of Point of View's
`ammo.cpp`, 55 of Wanted's `aflock.cpp`, about 50 of Wanted's `bot.cpp`,
and the whole section sequence of 67 of 69 objects of Gunman's client; in
They Hunger's `hl_wpn_glock.obj` the rule gave a window of 325 to 403 bytes
of header IL, and a sweep fitted from 326 to 401 and failed at 321 and
406).

C1XX emits a file's functions in two streams:

- **Own items**, in IL order: the file's non-inline, non-static functions,
  the template instantiations written at the end of the file, and the `_$E`
  static initialisers.
- **Deferred items**: header inline functions (vtable-only virtuals,
  implicit copy constructors and `operator=`, template members), inline and
  `static` functions of the `.cpp`, and `_$E` exception-handling functions. A
  deferred item becomes *needed* when an emitted function, vtable or datum
  first refers to it.

The rule: after own item i, every needed deferred item whose IL position v is
below E_i is emitted, in order of v; E_i is the IL offset where own item i+1
starts. Items still waiting at the end follow the last own item. An item
needed only after the cursor has passed it is emitted right after the item
that needs it.

Where v comes from depends on the header and the `.pch`:

| The header is | Its inlines land |
|---|---|
| not in the `.pch` the file found | right after the first function that needs them |
| in the `.pch`, and the file created it | at the cursor; v = p + B, with p the item's offset in the header region and B the IL offset where the `.pch` ends |
| in the `.pch`, and the file reused it | at the cursor; v = p as `/Bk` reports it in that compile |

The **create and reuse constants**: if you measure the offsets in a compile
without `/YX` (easier, and the only way for a file that creates its `.pch`:
`/Bk` with `/YX` in create mode makes C1XX fail with C1001), the cursor rule
becomes "emit when p < E_i − O + C", with O the offset of the file's first
function. C is near 0 when the file creates its `.pch` and about 1010
(990 to 1050 over 50 variants) when it reuses one. Point of View's
`ammo.cpp`, T2DEF's `aflock.cpp` and Wanted's `aflock.cpp` create theirs,
and so does Wanted's `bot.cpp` when it is compiled once in the replayed
build session. Compiled twice, `bot.cpp` reuses its own `.pch` and gets the
constant of about 1010; then no text fits retail's order (`Length` and
`Length2D` would need more IL moved out of the early functions than inline
functions defined later can take). With one compile, three small inline
functions and the stand-in of the previous section do. That decided the
session.

Consequences:

- Base classes' inlines come first, since they sit earlier in the `.pch`.
- Each function's IL moves the cursor. Point of View's `ammo.cpp`: 5,580
  compiles of blank lines, empty statements and `{}` all turned one dial and
  gave 27 orders; retail's needed the cursor after `GetSpriteList` between
  27,902 and 27,916.
- The include list of one file moves inlines in the next: it decides what
  `.pch` the next file finds.
- `/Zi` on or off changes the order too (in They Hunger's client
  `hl_weapons.cpp` it moves the cursor by 400 to 450 bytes).
- **A creating file pins the headers.** In an object whose file creates the
  `.pch`, every header inline's place depends on the IL of all header
  inlines before it. In They Hunger's server, 71 bytes more inline IL at
  the end of `extdll.h`, `util.h`, `schedule.h`, `basemonster.h` or
  `cbase.h` already move functions in `aflock.obj`, `buttons.obj` or
  `client.obj`. So the objects of creating files hold the shared headers'
  inline IL to a narrow range, and they locate IL that is missing: what
  `hl_wpn_glock.obj` lacked had to sit in `weapons.h` between
  `CBasePlayerItem` and the class whose inlines came out early, or be
  surplus in the file's own text.
- **A header edit changes the `.pch` chain.** A `.pch` that holds a header
  with a newer time stamp is rebuilt by the next file that meets it
  ([reproducing.md](reproducing.md#precompiled-header-state-full-builds-and-build-sessions)).
  After a header edit the IDE recompiles the header's dependents in project
  order, the first of them starting from a stale `.pch`, and the states
  that follow differ from a full build's. Five objects of They Hunger's
  server show that for `weapons.h`.

**How to use it:**

1. Compile the file with `/Bk<base>` (without `/YX` if it creates its
   `.pch`) and read the `ex` offsets of every function and every header
   inline it uses.
2. Write down retail's emission order, from the addresses of the functions
   that survive. Exports help: Point of View's client exports all 398 VGUI
   template members, which shows the order of 111 that LINK didn't fold.
3. For each boundary in retail's order, work out the window of IL that each
   own function must end in.
4. Fix structure first (which header declares a class, whether a member is
   inline). Gunman's `effectlist.h`: `CEffectList::Remove` is an inline in
   front of `Add` (`a1cc15c`); `dmlgrenade.h` sits inside the `.pch` right
   after `effects.h` (`feefcf2`); `CBeamGun` is declared in a header, like
   the other weapons (`89e2ac8`).
5. Only then reach for text that compiles to nothing, as in the previous
   section.

## Section order and COMDAT placement

Mostly **Observed**; the parts that follow from the emission rule above are
exact.

Inside one object:

- Top-level non-inline functions are emitted in source order. VC6 never
  reorders them (Point of View, Gunman and They Hunger all define
  `Sys_Error` before `GetGameDescription` in `client.cpp`, as retail places
  it).
- Inline functions defined in a class body are emitted after the first
  function that instantiates the class's vtable (usually the entity
  factory), base classes first. Declared in a header inside the `.pch`, they
  follow the cursor instead; declared in the `.cpp`, they come right after
  the factory.
- `static` functions are deferred: after their first caller, or at the end.
  Gunman's `TEXTURETYPE_Check` and `RocketDecal` sit where they are defined in
  retail, so they are not `static` (`a1475d7`).
- An override defined at the end of a `.cpp` lands just before the header
  inlines VC6 puts at the end of the object.
- Overloaded virtuals take their vtable slots in reverse declaration order
  (`FVisible( const Vector & )` before `FVisible( CBaseEntity * )`).
- A sparse switch's jump and index tables follow its function in `.text`.
- Data: each object's initialised data comes first, in definition order,
  then its string literals, in reverse order of their use in the code. Data
  of 8 bytes or more is 8-aligned. So data that follows another object's
  strings marks a new object: Gunman's `gGibModels` table proves an object
  with no code (`4e17f4e`), and a save table after a class's strings proves a
  second object (`2df8cb1`).
- A string literal first used inside an `EXPORT` member defined in the class
  body is exported (Gunman `320d5ba`).

Across objects:

- LINK places objects in link order, which is project order. The IDE keeps a
  `.dsp`'s files sorted by name, so a mod's new files sort into the gaps
  their code fills; Gunman's own block is in insertion order instead. Library
  members come after, in load order.
- An inline function emitted by several objects stays with the first in
  link order. Adding one header to one file can move kilobytes:
  `vgui_TeamFortressViewport.h` in Gunman's `ev_hldm.cpp` moved 14 KB of VGUI
  inlines into `ev_hldm.obj`.
- A float constant (`__real@...`) or string literal stays at its first
  definition in link order, even when that object's use of it is in a dead
  function. A constant in the wrong object's block is evidence of dead code
  (Gunman's double 3.0, `6e7096e`).

## LINK's identical-code folding

**Exact** for LINK 6.00.8168 on the VC6 CD's runtime. The rules come from
LINK's `/VERBOSE` output and a copy of the C runtime that logs every `qsort`
call; a simulator built from them predicts every address of our own links
(all 857 checked in Point of View's client).

With `/OPT:ICF`, LINK keeps one copy of each set of functions that have the
same bytes and relocations. It is on by default without `/debug`, so the five
clients fold and the five servers (which link with `/debug`) don't. Which copy
it keeps decides where every function after it lands.

1. **The array.** LINK collects every foldable code COMDAT: objects in link
   order, then library members in load order; inside an object in symbol
   table order, which is section order; external functions at their first
   definition; `static` ones too, except `_$E` initialisers; an associated
   `.text$x` section after its parent.
2. **Dead ones stay in it.** `/OPT:REF` marks unreferenced COMDATs dead
   (`EXPORT`ed ones count as referenced), but they remain in the array.
   LINK still resolves their references: an external that only a dead
   function uses must exist, or the link fails with LNK2001.
3. **The sort.** LINK sorts the array by (raw size, number of relocations)
   with the C runtime's `qsort`.
4. **The choice.** In each run of equal keys, the first *live* section of
   each identical group, in sorted order, is kept. The kept copy stays at its
   own place in its own object; the others vanish.
5. **Relocations.** Functions with relocations are identical only if their
   targets are; LINK makes two passes, using groups folded earlier in sorted
   order (T2DEF's client needed this limit).
6. An inline and a non-inline copy never fold together.
7. The COMDAT record's checksum takes part in "identical". LIBCMT's
   `_CallMemberFunction2` has the bytes of `_CallMemberFunction0` and `1`
   but another checksum, and LINK keeps it apart (They Hunger's client).
8. In the image, LINK groups contributions by section name: the `.text` of
   every object first, then all `.text$x` (They Hunger's client; with this
   and rule 7 the model placed 1,745 of 1,745 named sections of our own
   link).

So code that leaves no byte in the image still moves the ties, and so does
the C runtime: Wine's builtin `qsort` breaks ties differently from the VC6
CD's. With the CD's runtime under LINK, Point of View's client went from 38%
to 88% of `.text` at retail's addresses with no source change.

**The 1998 `qsort`** (**Exact**: a C port of it reproduces our links): the pivot is the middle
element of each partition, so the first pivot is the middle of the whole
array, and the number of entries inserted before the middle picks it. In a
partition of equal keys the middle element jumps to the front. The
permutation depends only on the sequence of keys, not on which section holds
a key. Entries inserted left of the first split, with keys no larger than the
pivot's, don't change the order of the right half.

**How to use it:**

1. Get the runtime right first ([reproducing.md](reproducing.md#import-table-order)).
2. Build the array from your own objects by the rules above, sort it with a
   port of the 1998 `qsort`, and check that the model reproduces your own
   link exactly. It should, to the byte.
3. Read retail's kept copies. Each one says that, in retail's array, that
   member came first among the live copies in its run: a fact about entries
   the image doesn't show.
4. Find a dead entry (size, relocation count, position) that explains them,
   and judge each candidate by the image, not by a list of kept names: "retail
   kept X" only means the kept copy sits in X's gap, and several members can
   fit one gap.
5. Use the other evidence for dead code: the [`.reloc` size](reproducing.md#the-reloc-size)
   gives its count of absolute fixups, constant blocks give its constants,
   and the emission order of the object gives the IL windows it must fit.

Examples:

- **Point of View's client** (`469339a`): a dead function of (80 bytes,
  4 relocations) right after `CBaseEntity::Killed` in `hl_weapons.obj`.
  The first guess, (48, 3), put 49 of 50 groups right and fit nowhere once
  `ammo.obj`'s order was fixed; `.reloc`'s size then said four fixups.
  Committed as `__pov_unknown_stub`, name and body ours.
- **Wanted's client** (`5f7d773`): one 16-byte dead function with no
  relocations after `HUD_PostRunCmd`, necessary and sufficient for 841 of 841
  sections at retail's addresses. No other single insertion, deletion or swap
  fits.
- **T2DEF's client** (`d03b7a2`): no dead code; moving one class
  declaration in `vgui_TeamFortressViewport.h` put that object's inlines in
  retail's order, and with them the kept copies of 11 functions.
- **They Hunger's client** (`d55d13c`, `4040aea`, `e553179`): no invented
  dead function. At first 29 of 65 folded groups kept another copy than
  retail's, and three inputs to the one sort were wrong. The stub file
  needed SDK 2.2b's order and two of its dead stubs (without either one,
  101,466 bytes of the image differ; without both, 214,594). `ammo.obj`
  and `hl_weapons.obj` needed retail's section order, which is a matter of
  [IL](#text-that-compiles-to-nothing-still-counts), not of dead code: one
  more dead function of any of 98 shapes in `hl_weapons.obj` moved at least
  36,169 bytes in the model. With all three right, every kept copy is
  retail's.

**Why the image can't always tell.** In Gunman's client, 24 of 84 classes of
identical functions keep another copy than retail's. Searches over single
dead entries of any key at any place (6.8 million arrays), deletions of the
client's real dead code (1.3 million subsets of 32 candidate functions),
free reordering of the invisible entries inside their objects, and the
alternative `.pch` states of every file found none that gives retail's
copies. A fit with invented entries could be found, but the outcome
of the classes carries about 63 bits and each freely placed entry about 11,
so such a fit would be one of very many and not evidence. The array retail
sorted differed in entries the image doesn't show. That is where we stopped.

## `.bss` order

**Exact** (3,072 test names, 42 random sets, every object of Wanted's
server).

Uninitialised globals are laid out by walking a 1024-bucket hash of their
names: `h = h*4 + (h >> 4) + c` over the name (32-bit), bucket
`(h ^ (h >> 16)) % 1024`, ascending; inside a bucket the latest declaration
first; then zero-initialised data in definition order. The name is the plain
identifier for file-scope data and the decorated name for function statics
and static class members, so a class name can decide where a function's
statics land (Wanted `77d79d0`). When retail doesn't give a global's name,
the globals around it whose names are known give a window of buckets its
name must hash into (They Hunger `e90c20f`: five windows for five objects,
and no naming scheme that fits them all). The details, a Python version and
the examples are in [reproducing.md](reproducing.md#bss-order).

## What leaves no trace

The image keeps every function that survives the link, every byte of data,
and the order of both. It doesn't keep:

- **Names**, except where they reach the image: exports, save-table strings,
  other string literals, the `.bss` hash, the `/Od` slot hash, and the
  decorated names of static members through the `.bss` hash. Class, member,
  local and file names are otherwise free. Mark the ones you make up.
- **Text that compiles to nothing:** comments, layout, unused inline
  functions, `if ( 0 )` blocks, dead statements. They show only through the
  inliner's budget or the IL cursor, never as which text it was.
- **Objects that compiled to nothing.** Only the Rich header counts them.
- **Dead functions** in a `/OPT:REF` link. Only their (size, relocations)
  key acts, through the fold sort; `.reloc`'s size gives their absolute
  fixups; constants they share with live code stay at their object.
- **The order of invisible entries** inside an object: folded copies and dead
  functions between the visible ones.
- **Build history**, beyond the PDB's creation time and age, the final link
  time and the `.pch` states the objects show: which files were recompiled,
  and when.
- **Preprocessor directives.** Compiled code has none. A conditional in a
  reconstruction can only come from the SDK text a file copies, or from a
  file compiled into both DLLs whose two compiled forms differ.
- **Which C runtime the original linker ran on**, except through how it broke
  ties.
- **Declarations.** How many symbols the headers declared shows in the few
  functions whose reload order follows the count modulo 64
  ([above](#reloads-at-a-loop-end-symbol-numbers)); what they declared
  doesn't show at all.
- **Compiler state the final code doesn't show:** how far the scheduler
  hoisted a call's set-up in a long block, or the reload order at a loop's
  exit that moves with the function's constants modulo 64. Three of Gunman's
  client functions stay a few lines off for this reason.
- **The front end's version** when only the back end stamps the object
  (T2DEF's client: was `/Ob0` in the project, or did Service Pack 4's own
  front end not inline?).

## A working order

What worked for five mods, in order:

1. Pick the compiler from the Rich header and the base from SDK builds.
2. Match functions with an address-masked matcher, then check call targets,
   constants, strings, vtables and data separately.
3. Lay the objects out: link order, object boundaries from the data, file
   names that sort into the gaps.
4. Make sections identical at the same addresses, not just functions:
   size first (one byte early shifts the rest), then data and `.bss`.
5. Fix where header inlines land: include lists, the `.pch` chain, the IL
   cursor.
6. For a client, LINK's kept copies: runtime, then dead entries, judged by
   the image.
7. The file-level parts: [reproducing.md](reproducing.md).
8. Write down every line that exists only to steer the compiler, with its
   evidence, and mark it `// sic`.
