<?php
/**
 * @file
 * Integration with Views.
 */

/**
 * Helper function startsWith
 */
function _views_filters_selective_startsWith($haystack, $needle)
{
    return $needle === "" || strpos($haystack, $needle) === 0;
}

/**
 * Helper function endsWith
 */
function _views_filters_selective_endsWith($haystack, $needle)
{
    return $needle === "" || substr($haystack, -strlen($needle)) === $needle;
}

/**
 * Implementation of hook_views_data_alter()
 */
function views_filters_selective_views_data_alter(&$data) {
  foreach ($data as $table_name => $table_info) {
    foreach ($table_info as $field_name => $field_info) {
      if (!empty($field_info['filter']['handler']) && $field_info['filter']['handler'] != 'views_handler_filter_selective') {
            $_field_name =  $field_info['filter']['field_name'];
            //Ignore translated, format and delta to reduce the amount of available filters in UI
            if(_views_filters_selective_endsWith($field_name, '_i18n') || _views_filters_selective_endsWith($field_name, '_format') || $field_name == 'delta'){
                continue;
            }
            $_field_real_field = $field_info['filter']['real field'];
            $real_field = empty($_field_name) ?  (empty($_field_real_field) && !empty($field_info['field']) ? $field_name :$_field_real_field) : $_field_name;
            if(empty($real_field)){
                //Only allow selective filters on fields that can be added to query
                continue;
            }
            $selective = $field_info;
            $selective['title'] = (!empty($field_info['filter']['title']) ? $field_info['filter']['title'] : $field_info['title']) . ' (selective)';
            $selective['filter']['handler'] = 'views_handler_filter_selective';
            $selective['filter']['proxy'] = $field_info['filter']['handler'];
            $selective['filter']['field'] = $field_name;
            $selective['filter']['field_base'] = $real_field;
            unset($selective['argument'], $selective['field'], $selective['relationship'], $selective['sort'], $selective['filter']['title']);
            $data[$table_name][$field_name . '_selective'] = $selective;
      }
    }
  }
}

