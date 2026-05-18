all:
	clear
	g++ main.cpp library.cpp book.cpp page.cpp -I . -o Final03
	./Final03

run: 
	./Final03

clean:
	rm -rf $(O)