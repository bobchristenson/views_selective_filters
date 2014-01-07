Allows to have an exposed filter only show options that belong to result set.

To make it work:

[0] Patch Views core if necessary, check INSTALL.txt.
[1] Enable the module.
[2] Add a filter of type "selective" to your view, take special note of the Administrative Name for the filter.
[3] Add a field to the view that uses the same base field as the filter, and give this field the SAME ADMINISTRATIVE NAME AS THE FILTER.
[3.1] Configure the field output in [3] as you wish, this will be what is shown as the filter's available options.
[3.2] If you do not want this field to be shown in the result set, just mark it as "Hide From Display".

You can get 3 types of errors with this module:

(a) You did not properly match a filter and a field through their Administrative Name.
(b) You properly matched filter and field through Administrative name, but their base field is not the same. 
You get the same error text as in (a).
(c) You choose a field whose distinct value has more than 99 (limit is hardcoded) different possible values, 
obviously this field is not the kind of field you would use for a selective filter.

KNOWN ISSUES: When one of the available options in the result set is NULL, this will interpret the value as an
empty string when constructing the query, so it will not work as expected. To overcome the issue, NULL values are
excluded from available options! 