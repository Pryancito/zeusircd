#include "include.h"
#include <map>
using namespace std;

// Constructor por defecto
template<typename T>
List<T>::List()
{
    m_num_nodes = 0;
    m_head = NULL;
    m_tail = NULL;
}

// Insertar by Pryan
template<typename T>
void List<T>::add(T data_)
{
	Node<T> *current = new Node<T> (data_);
	if (m_head == NULL)
    {
        m_head = current;
    }
    else
    {
        m_tail -> next = current;
        current -> prev = m_tail;
    }
    m_tail = current;
	jash[data_] = current;
	m_num_nodes++;
}

// Eliminar todos los nodos
template<typename T>
void List<T>::del_all()
{
	if (m_num_nodes == 0)
		return;
    m_head->delete_all();
    m_head = NULL;
    m_tail = NULL;
    m_num_nodes = 0;
}

// Eliminar by Pryan
template<typename T>
void List<T>::del(T data_)
{
	if (m_num_nodes == 0) {
		return;
	}
	else if (!jash[data_]) {
		return;
	}
	
    Node<T> *current = jash[data_];
    Node<T> *previous = current->prev;
	   
    if(m_head == m_tail)
    {
        m_head = NULL;
        m_tail = NULL;
        delete current;
        m_num_nodes--;
        jash.erase(data_);
        return;
    }
    else if (current == m_head)
    {
        m_head = current->next;
        m_head -> prev = NULL;
        delete current;
        m_num_nodes--;
        jash.erase(data_);
        return;
    }
    else if (current == m_tail)
    {
        previous -> next = NULL;
        m_tail = previous;
        delete current;
        m_num_nodes--;
        jash.erase(data_);
        return;
    }
    else
    {
        (previous -> next) = (current -> next);
        (current -> next) -> prev = previous;
        delete current;
        m_num_nodes--;
        jash.erase(data_);
        return;
    }
}

// Siguiente en la Lista
template<typename T>
T List<T>::next(T data_)
{
	Node<T> *datos = jash[data_];
	if (!m_head) {
		return NULL;
	}
		
	else if (!datos) {
		return NULL;
	}
	else if (datos == NULL) {
		return NULL;
	}
	else if (datos->next != NULL) {
		return datos->next->data;
	}
	else {
		return NULL;
	}
}

// Principio de la Lista
template<typename T>
T List<T>::first()
{
	Node<T> *temp = m_head;
    if (!m_head) {
        return NULL;
    } else {
        return temp->data;
    }
}

// Numero de entradas
template<typename T>
int List<T>::count()
{
	return m_num_nodes;
}

// Buscar el dato de un nodo

template<typename T>
T List<T>::search(T data_)
{
	Node<T> *datos = jash[data_];
	if (!m_head) {
		return NULL;
	}
	else if (!datos) {
		return NULL;
	}
	else if (datos->data == data_) {
		return datos->data;
	}
	else {
		return NULL;
	}
}

template<typename T>
List<T>::~List() {}
