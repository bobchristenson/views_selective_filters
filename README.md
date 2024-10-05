Views Selective Filters
=======================

Allows to have an exposed filter only show options that belong to result set.

This module will replicate existing fields that are available.  The fields that
this module adds are identified with the suffix "(selective)".

Installation and Usage
----------------------

- Install this module using the [official Backdrop CMS instructions](https://backdropcms.org/guide/modules)
- Add a field to the view that you want to use as a distinct field.
- Configure the field output as you wish.
- If you do not want this field to be shown in the result set,
  just check the box in the field settings to "Exclude from display".
- Add a field with the sufffix "(selective)" to the filters on your view, and
  in the filter settings select the field you configured earlier. If the field
  is not showing, that means that the filter and field you are trying to match
  are not compatible.

You can get 3 types of errors with this module:

  (a) You did not properly match a filter and a field.
  (b) You properly matched filter and field, but the base field is not the same.
      You get the same error text as in (a).
  (c) You choose a field whose distinct value has more than the limit (100 by
      default) of different possible values; obviously this field is not the
      kind of field you would use for a selective filter.

License
-------

This project is GPL v2 software. See the LICENSE.txt file in this directory for
complete text.


Current Maintainers
-------------------

- [Bob Christenson](https://github.com/bobchristenson)
- [Laryn Kragt Bakker](https://github.com/laryn)
- Seeking additional maintainers.


Credits
-------

- Originally written for Drupal by [David Garcia](https://www.drupal.org/u/david_garcia).
- Ported to Backdrop CMS by [Bob Christenson](https://github.com/bobchristenson)
  and [Herb Dool](https://github.com/herbdool).
