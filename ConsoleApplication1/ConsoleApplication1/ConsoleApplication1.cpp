#include <iostream>

struct element {
    element* forwardElement = nullptr;
    char character;
    element* backElement = nullptr;

    element(char inCharacter) { character = inCharacter; }
};

void LinkedListInput(element* prevElement, element* newElement)
{
    newElement->forwardElement = prevElement->forwardElement;
    if (newElement->forwardElement != nullptr)
    {
        newElement->forwardElement->backElement = newElement;
    }
    prevElement->forwardElement = newElement;
    newElement->backElement = prevElement;
}

int main()
{
    element* e1 = new element('A');
    element* e2 = new element('B');
    element* e3 = new element('C');

    e1->forwardElement = e2;
    e2->forwardElement = e3;
    e2->backElement = e1;
    e3->backElement = e2;
}
