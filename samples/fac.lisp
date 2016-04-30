(define fac (lambda (n)
	(if (= n 1)
		n
		(* n (fac (- n 1)))
	)
))