*** NOTES FOR FOLKS WHO WANT TO CREATE THEIR OWN CUBE MODELS ***

Each cube model has 7 filters, numbered 0-6.
Each filter has a model:
Currently only two Models: 0-Biquad,2-Comb,3-Modal (1 is coming soon)

Each filter has a Non-liearity structure
Structures: 0-None, 1-Non-linear states,2-Non-linear feedback,3-Nonlinear states and feeback

Each filter has a Non-linearity function
Functions: 0-Soft Clipping, 1-Hard Clipping, 2-Tanh Clipping,3 Double Soft Clipping

Each filter has a type:
Filter Types: 0-Low Pass, 1-High Pass, 2-Band Pass, 3-Notch, 4-Peak, 5-Low Shelf, 6-High Shelf, 7-All Pass.
! Types 4 and higher have not been tested, but should work !

The filters can be placed in serial and parallel in any combination
Filters on the same "level" are in parallel.
ie. all filters on level 0 are in parallel, then fed into the filter(s) on level 1, etc..
This means there can potentially be up to 7 levels (0-6)
Levels should be consecutive starting with 0.
If a filter's level is negative, it is not used
! If the levels are not consecutive, there is no guarantee what will happen !

Each model has 8 vertexes representing the corners of the cube.
! Don't change the x,y,z values !
For each vertex, you can set the Frequency Cutoff, Q, drive and gain for each of the 7 filters
each vertex has a makeup gain parameter so you can adjust overall volume
All values in the vertex are floating point values, so there *MUST* be a decimal point, even if it is .0
! IF there is not a decimal point, the model will break the filter and you will need to restart VCV !

If you come up with a cube model you really like, send it to me: almostEric@frozenwastelandmodules.com
It might get added to the default cube model collection
