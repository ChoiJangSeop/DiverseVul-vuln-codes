TEST_CASE("Cyclic grammer test", "[general]")
{
    Definition PARENT;
    Definition CHILD;

    PARENT <= seq(CHILD);
    CHILD  <= seq(PARENT);
}