### Lithp

This is a (slow) interpreter for a tiny subset of Lisp.

Use `set` to bind variables. This is how you bind functions to variables as well, a-la Scheme. Use `quote` to quote things, as the apostrophe syntactic sugar isn't available yet. `lambda` creates functions. `if` branches to either consequence based on the truth of the condition. `=` returns NIL or T based on the equality of two numbers. `progn` evaluates each argument in order, and returns the evaluated value of the last one. `print` will print a value.

Example:

```
(set factorial
     (lambda (x)
	   (if (= x 0)
	       1
		   (* x (factorial (- x 1))))))
```

The interpreter will load into a REPL by default. Add file-names on the command line to load files in. Use `(quit)` to leave the REPL.
