# Iteration and Recursion Workshop

Traditional programming and idiomatic APL written in a data-parallel style can sometimes 
feel incompatible and impossible to use together. Dyalog APL provides a number of 
features around traditional iteration that can be used to great effect or that can 
greatly undermine APL's biggest advantages. Understanding how traditional concepts 
like iteration and recursion fit into the idiomatic APL landscape can greatly improve 
the architectural clarity and maintainability of your code, as well as giving you 
a better overview of large APL applications.

In this workshop you will explore patterns for writing iterative, recursive, and 
stateful code in APL. There will be a focus on how to take best advantage of the 
many constructs for control flow available in Dyalog, such as structured statements, 
labels, and recursion; you will learn how to use these features to write programs 
that are easy to read, debug, and maintain while avoiding the pitfalls that can undermine 
the quality of your code.

## What is Iteration/Recursion?

For the purposes of this workshop, we are talking about iteration and recursion 
as a set of programming techniques for specifying the control flow of your
program (that is, what the next code your program should execute as it is 
executing) that deals with explicit looping (using goto/branch or structured 
statements like :While) and recursion (a function calling itself or calling 
another function that will eventually call itself) *as opposed to* the use 
of APL primitives. 

APL provides a set of built-in operators that provide for specific iteration 
patterns over your data, as follows:

* Each
* Reduce
* Scan
* Key
* Stencil
* Inner Product
* Outer Product

[Note that for the purposes of this workshop, we consider ⍣ (Power Limit)
to be an explicit looping construct.]

Each of these operators traverses an array in some specific way, and applies 
the given operand(s) to that data, and then recombines the results of applying 
the operand(s) in some specific way. 

In this workshop, we are going to focus on the forms of iteration and recursion 
in APL that constitute *explicit control flow*. 

## When should I use explicit control flow (iteration/recursion)? 

APL's core operators are very powerful. The vast majority of problems can be 
entirely or almost entirely solved using just the APL primitives and an 
appropriate application of APL operators. 

There are many benefits to using APL operators instead of explicit control flow. 
One of the primary reasons is that the iteration patterns of each operator are 
specific and static. You can't *mess up* the iteration, introduce a bug by 
traversing the data incorrectly, or somehow forget to terminate (a common fault). 

The APL operators also encompass the vast majority of commonly used 
iteration/traversal patterns. 

Why then, might we not want to use them, and instead use an explicit form 
of control flow? 

Fundamentally, APL's operators provide a sort of middle ground between no 
explicit control flow and completely arbitrary control flow. This means that 
they make some very specific trade-offs in their design to make them easier to 
use in most cases. Specifically, APL's operators all:

* Guarantee termination
* Guarantee that they traverse all the data in a specific way
* Guarantee that a specific operand is always applied to the data in the same 
  way every time

But there are times when you may not want to have these guarantees, because 
your problem *inherently* does not work that way. Generally speaking, whenever 
you have a problem where *the code that you should run, or even if you should 
run some code, depends on the intermediate results of calculating over the 
input data*, then you probably want to look at explicit control flow as a 
means of organizing the basic structure of your code. 

Said another way, whenever there are data dependencies in the control flow of 
your program, you probably want to use explicit control flow. 

As we will see, another reason you may want to use explicit control flow is 
if the specific iteration pattern that you want is not easily built out of the 
basic APL operators. This is a case where the control flow and the shape of 
the data you are working over is combined and partitioned based on intermediate 
results that doesn't match the way that the APL operators work. 

In short, if any of the following hold true, you may want to consider using 
explicit control flow in your code:

1. What code to run depends on past computations and new input data
2. When to stop running your code depends on either input data or intermediate results. 
3. The data needs to be split apart and recombined in ways that depends on the 
   data itself. 

## The major iterative and recursive patterns

There are three major design patterns that we suggest considering when any of 
the previously mentioned three conditions hold:

1. Sequence-based Enumeration (State Machines)
2. Fixed-points (Power Limit, While loops, etc.)
3. Divide-and-Conquer (Recursion, typically)

When should you use each pattern? 

*Sequence Enumeration* is useful when the code that you need to run depends
on the input you see and possibly what previous input you have seen, and 
where you either need to produce output without seeing all the input, or 
where you can't know in advance when the next piece of input will arrive, but 
you need to do something anyways. It is also the recommend pattern for cases 
when your application may need to continue to run, processing input, while 
never terminating (at least not normally). Examples of this kind of code include 
GUIs, network servers, event-driven applications, etc. 

*Fixed-points* are good to use whenever you know the code you want to run, 
but you don't know for how long, or when you will finally reach the answer. 
This may mean that the code might terminate before you have gone through all 
the data, or it may mean that you must continue to refine some working data 
until some condition is met. Examples of this sort of application include 
optimization problems, solvers, and machine learning applications. 

The *Divide-and-Conquer* pattern is useful whenever the working set of your 
problem needs to be split up for some reason. This might be for performance 
purposes (cache optimizations, tiling, etc.) or it might be because the 
algorithm itself needs to split up the data in specific ways and then 
recombine that data, remembering how the data was split up. It may be that 
the specific way in which you split up the problem depends on the data you 
have just seen, such that you cannot know ahead of time how you will split the 
data up. Many search algorithms work like this, such as depth-first and 
bread-first search. Quicksort is probably the most famous divide-and-conquer 
algorithm in existence, and Dijkstra's algorithm for finding your way through 
a graph can also be done in this fashion. 

The *divide-and-conquer* method is interesting because it is the closest to 
APL operators, given that it generally is expected to terminate and you may often 
know in advance that you will traverse all the data (such as in the quicksort 
algorithm). There are even interesting ways to combine recursion with the 
APL operators to do this sort of work. However, it would be difficult to 
make an APL operator to do divide-and-conquer algorithms, because often the 
kinds of traversals that you make are not specific iteration patterns, but 
require you to decide how to call your code based on the shape of the data. 
This would make defining a "divide and conquer" operator very difficult, and 
why it is easier to just use explicit control flow to do this sort of programming. 

## Practicing these Patterns

We have three practice problems for you to work through each different design 
pattern:

1. A home alarm system to practice sequence enumeration
2. The quickhull algorithm for finding a convex hull of a set of vertices/points, 
   to practice fixed-points
3. A multi-grid solver for 3D Poisson equations using hierarchical grid refinement
   to practice your divide-and-conquer skills
4. (Bonus) FlashAttention to see how cache optimizations can improve memory and 
   execution performance
   
We will introduce each of these problems together, and then you can go ahead 
and play with the ones you find most interesting at your leisure! 
