all:
	gcc main.c -o exe -larchive

run:
	./exe CJ9486_a0cdf39b35e6_Rc_2024_06_24_Td_2024_06_24_NFF_TotalWine_513_WestPlano_STR.tgz

clean:
	rm exe
