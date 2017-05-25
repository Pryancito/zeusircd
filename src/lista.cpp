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

// Concatenar a otra List
template<typename T>
void List<T>::concat(List list)
{
    Node<T> *temp2 = list.m_head;
 
    while (temp2) {
        add_end(temp2->data);
        temp2 = temp2->next;
    }
}
 
// Eliminar todos los nodos
template<typename T>
void List<T>::del_all()
{
    m_head->delete_all();
    m_head = 0;
}

// Eliminar by Pryan
template<typename T>
void List<T>::del(T data_)
{
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

// Usado por el método intersección
template<typename T>
void insert_sort(T a[], int size)
{
    T temp;
    for (int i = 0; i < size; i++) {
        for (int j = i-1; j>= 0 && a[j+1] < a[j]; j--) {
            temp = a[j+1];
            a[j+1] = a[j];
            a[j] = temp;
        }
    }
}
 
// Números que coinciden en 2 Lists
template<typename T>
void List<T>::intersection(List list_2)
{
    Node<T> *temp = m_head;
    Node<T> *temp2 = list_2.m_head;
 
    // Creo otra Lista
    List intersection_list;
 
    int num_nodes_2 = list_2.m_num_nodes;
    int num_inter = 0;
 
    // Creo 2 vectores dinámicos
    T *v1 = new T[m_num_nodes];
    T *v2 = new T[num_nodes_2];
 
    // Lleno los vectores v1 y v2 con los datas de la lista original y segunda lista respectivamente
    int i = 0;
 
    while (temp) {
        v1[i] = temp->data;
        temp = temp->next;
        i++;
    }
 
    int j = 0;
 
    while (temp2) {
        v2[j] = temp2->data;
        temp2 = temp2->next;
        j++;
    }
 
    // Ordeno los vectores
    insert_sort(v1, m_num_nodes);
    insert_sort(v2, num_nodes_2);
 
    // Índice del 1er vector (v1)
    int v1_i = 0;
 
    // Índice del 2do vector (v2)
    int v2_i = 0;
 
  // Mientras no haya terminado de recorrer ambas Lists
  while (v1_i < m_num_nodes && v2_i < num_nodes_2) {
      if (v1[v1_i] == v2[v2_i]) {
          intersection_list.add_end(v1[v1_i]);
          v1_i++;
          v2_i++;
          num_inter++;
      } else if (v1[v1_i] < v2[v2_i]) {
          v1_i++;
      } else {
          v2_i++;
      }
  }
 
  // Solo si hay alguna intersección imprimo la nueva lista creada
  if (num_inter > 0) {
      cout << "Existen " << num_inter << " intersecciones " << endl;
      intersection_list.print();
  } else {
      cout << "No hay intersección en ambas listas" << endl;
  }
}
 
// Invertir la lista
template<typename T>
void List<T>::invert()
{
    Node<T> *prev = NULL;
    Node<T> *next = NULL;
    Node<T> *temp = m_head;
 
    while (temp) {
        next = temp->next;
        temp->next = prev;
        prev = temp;
        temp = next;
    }
    m_head = prev;
}

// Imprimir la Lista
template<typename T>
void List<T>::print()
{
    Node<T> *temp = m_head;
    if (!m_head) {
        cout << "La Lista está vacía " << endl;
    } else {
        while (temp) {
            temp->print();
            if (!temp->next) cout << "NULL";
                temp = temp->next;
        }
  }
  cout << endl << endl;
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
/*	
	Node<T> *datos = search(data_);
    if (!m_head) {
        return NULL;
    } else if (datos != NULL) {
        if (datos->next == NULL) return NULL;
        else return datos->next->data;
    } else return NULL;*/
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
/*
template<typename T>
Node<T> *List<T>::search(T data_)
{
    Node<T> *temp = m_head;
 
    while (temp != NULL) {
        if (temp->data == data_)
            return temp;
        else
        	temp = temp->next;
    }
    return NULL;
}*/
 
// Ordenar de manera ascendente
template<typename T>
void List<T>::sort()
{
    T temp_data;
    Node<T> *aux_node = m_head;
    Node<T> *temp = aux_node;
 
    while (aux_node) {
        temp = aux_node;
 
        while (temp->next) {
            temp = temp->next;
 
            if (aux_node->data > temp->data) {
                temp_data = aux_node->data;
                aux_node->data = temp->data;
                temp->data = temp_data;
            }
        }
 
        aux_node = aux_node->next;
    }
}

template<typename T>
List<T>::~List() {}
