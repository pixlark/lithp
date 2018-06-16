(set factorial
	 (lambda (x)
	   (if (= x 0)
		   1
		 (* x (factorial (+ x -1))))))

(set fibonacci
	 (lambda (a b lim)
	   (if (= lim 0)
		   b
		 (fibonacci b (+ a b) (- lim 1)))))
