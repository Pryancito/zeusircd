#include "include.h"
#include <map>
using namespace std;

// Constructor por defecto
template<typename T>
List<T>::List()
{
    m_num_nodes = 0;
    m_head = NULL;
}

// Insertar by Pryan
template<typename T>
void List<T>::add(T data_)
{
	Node<T> *new_node = new Node<T> (data_);
	Node<T> *temp = m_head;
	
	if (m_head == NULL) {
		m_head = new_node;
	} else {
		while (temp->next != NULL) {
			temp = temp->next;
		}
		temp->next = new_node;
	}
	jash[data_] = new_node;
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
    m_num_nodes = 0;
}

// Eliminar by Pryan
template<typename T>
void List<T>::del(T data_)
{
	if (m_num_nodes == 0)
		return;
	else if (!jash[data_])
		return;
		
    Node<T> *temp = m_head;
    Node<T> *temp1 = m_head->next;
	   
	if (m_head->data == data_) {
        m_head = temp->next;
        m_num_nodes--;
        jash.erase(data_);
        return;
    } else {
        while (temp1) {
            if (temp1->data == data_) {
                Node<T> *aux_node = temp1;
                temp->next = temp1->next;
                delete aux_node;
                m_num_nodes--;
                jash.erase(data_);
                return;
            }
            temp = temp->next;
            temp1 = temp1->next;
        }
    }
}

// Siguiente en la Lista
template<typename T>
T List<T>::next(T data_)
{
	Node<T> *datos = jash[data_];
	if (!m_head)
		return NULL;
	else if (!datos)
		return NULL;
	else if (datos == NULL)
		return NULL;
	else if (datos->next != NULL)
		return datos->next->data;
	else
		return NULL;
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
	if (!m_head)
		return NULL;
	else if (!datos)
		return NULL;
	else if (datos->data == data_)
		return datos->data;
	else
		return NULL;
}

template<typename T>
List<T>::~List() {}
