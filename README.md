# ccmdliner
A Light-weight Commandline Parser implemented in Modern C++

### Example 
``` cpp
    commandline op;
    long long j=1;
    op.overview = "commandtool Tool";
    op.add_action("list", "List all detected ATA devices",3);
    op.add_option("size", "Set size to ATA devices");
    try
    {
        auto r = op.parse(argc, argv);
        cout << r.action() << endl; 
        if (auto [ok, val] = r.option<long long>("size"); ok)
            cout << val << endl;
    }
    catch (exception& e)
    {
        cout << e.what() << endl;
    }
```
