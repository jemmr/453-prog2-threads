Jemma Arona, Luis Guzman

“How is this LWP library different from a real thread management library? Name 2 ways in which this LWP library API could be improved.”
  Unlike real thread libraries, this LWP library was written by a college student over the course of two days.
  The data structures are probably implemented inefficiently, and there are no defined limits on the number of created threads.
  There is little error handling, and it relies very heavily on its methods being called in certain, narrower, situations (eg lwp_wait() in the main thread only).
  It is (probably) not POSIX-compliant.

  Besides fixing the above, two ways we could improve the API are by including the following features:
  - Defining and handling multiple standard exit statuses.
  - Enhancing thread communication and synchronicity with preemption and mutex locks.

Luis was unavailable for the majority of the work period.
