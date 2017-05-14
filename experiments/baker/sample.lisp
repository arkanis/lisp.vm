(set! fac (lambda (n)
	(if (< n 2)
		1
		(* n (fac (- n 1)))
	)
))
(endless (fac 25))

(set! count (lambda (n)
	(if (< n 1)
		1
		(+ n (count (- n 1)))
	)
))

(count 100)

(set! loop (lambda () )