<?php

class views_handler_filter_selective extends views_handler_filter_in_operator {
  static $results;
  
  function views_handler_filter_selective(){
    //This filter showld always be exposed!
    $this->options['exposed'] = TRUE;
    //Do not allow to reduce options, makes no sense
    $this->options['expose']['reduce'] = FALSE;
  }

  function get_value_options() {
    $this->value_options = array();
    if (empty($this->view->selective_oids) && !empty($this->view->inited)) {
      $oids = $this->get_oids($this->definition['field_base'], $this->real_field, $this->field, $this->table, $this->table_alias);
      $this->value_options = $oids;
      //Use array_diff_key to remove options with "empty" values
      $this->value_options = array_diff_key($this->value_options , array("" => NULL)); 
    }
  }

  function value_form(&$form, &$form_state) {
    parent::value_form($form, $form_state);
    // Avoid the 'illegal values' Form API error.
    $form['value']['#validated'] = TRUE;
  }
  
  function options_form(&$form, &$form_state) {
      parent::options_form($form, $form_state);
      //Filter should always be exposed, show warning
      $form['expose_button'] = array('warning' => array(
                                          '#type' => 'markup',
                                          '#markup' => '<div class="messages warning">This filter is always exposed to users.</div>'
                                      ));
      //Change "All Values" to "Available at runtime"
      $form['value']['#options']['all'] = t('Actual values will be available at run-time');
      //Cannot group without values
      unset($form['group_button']);
  }
  
  function expose_form(&$form, &$form_state) {
      parent::expose_form($form, $form_state);
      unset($form['expose']['reduce']);
  }

  function query() {
    // Decode the values to restore special chars.
    $this->value = array_map('urldecode', $this->value);
    parent::query();
  }

  function get_oids($field_name, $real_field, $field, $table, $table_alias) {
    $exposed_input = isset($this->view->exposed_input) ? $this->view->exposed_input : array();
    $exposed_input = array_merge(array(), $exposed_input);
    unset($exposed_input[$field_name . '_selective']);
    $signature = md5(serialize(array(
      'name' => $this->view->name,
      'args' => $this->view->args,
      'input' => $exposed_input,
      'field_name' => $field_name
    )));
    if (empty(self::$results[$signature])) {
      $max_items = 100;
        
      // Clone the view and get all results.
      $view_copy = views_get_view($this->view->name);
      if (!$view_copy) return NULL;
      $view_copy->selective_oids = TRUE;
      $view_copy->set_exposed_input($exposed_input);
      $view_copy->set_arguments($this->view->args);
      
      // It is very easy to mess up with the field used for distinct and easily have thousands of elements.
      // For protection, limit result set to 100 items, anyways anything above 100 is not user friendly at all.
      $view_copy->set_items_per_page($max_items);
      
      // Remove paging, items per page and page number from context, just in case....
      if (isset($_GET['items_per_page'])) {
        $items_per_page = $_GET['items_per_page'];
        unset($_GET['items_per_page']);
      }
      if (isset($_GET[$field])) {
          $exposed_filter = $_GET[$field];
          unset($_GET[$field]);
      }
      if (isset($_GET['page'])) {
          $exposed_page = $_GET['page'];
          unset($_GET['page']);
      }
      
      // Remove "our" field from exposed input.
      unset($view_copy->exposed_input[$field]);
      
      $display_to_use = $this->view->current_display;
      
      // Manipulate specific display + default, we don't know if fields are overriden.
      $display = $view_copy->display[$display_to_use];
      $display_default = $view_copy->display['default'];
      
      // Remove all fields, except for the one we want the "distinct" to happen in.
      $display->display_options['fields'] = array_intersect_key($display->display_options['fields'],array($field_name => NULL));
      $display_default->display_options['fields'] = array_intersect_key($display->display_options['fields'],array($field_name => NULL));
      
      // Check to see if the user remembered to add correspondiend field to fields.
      if(empty($display->display_options['fields']) && empty($display_default->display_options['fields'])){
          drupal_set_message('Selective query filter {$field} must have corresponding field added to view.');
          return array();
      }
      
      // If main field is excluded form presentation, bring it back.
      // Set group type for handler to populate database relationships in query.
      if(isset($display->display_options['fields'][$field_name])){
          $display->display_options['fields'][$field_name]['exclude'] = 0;
          $display->display_options['fields'][$field_name]['group_type'] = 'group';
      }
      if(isset($display_default->display_options['fields'][$field_name])){
          $display_default->display_options['fields'][$field_name]['exclude'] = 0;
          $display_default->display_options['fields'][$field_name]['group_type'] = 'group';
      }
      
      // Remove all sorting, this will be included in the aggregate fields, we do not want that to happen.
      unset($display->display_options['sorts']);
      unset($display_default->display_options['sorts']);
      
      // Turn this into an aggregate query.
      $display->display_options['group_by'] = 1;
      $display->handler->options['group_by'] = 1;
      
      $display_default->display_options['group_by'] = 1;
      $display_default->handler->options['group_by'] = 1;
      
      // Run View.
      $view_copy->execute($this->view->current_display);

      // Reset conext parameters for real View.
      if (isset($items_per_page)) {
        $_GET['items_per_page'] = $items_per_page;
      }
      if (isset($exposed_filter)) {
          $_GET[$field] = $exposed_filter;
      }
      if (isset($exposed_page)) {
          $_GET['page'] = $exposed_page;
      }
      
      // Create array of objects for selector.
      $oids = array();
      $field_alias = $view_copy->query->field_aliases[$table][$real_field];
      foreach ($view_copy->result as $index => $row) {
          $key = $row->{$field_alias};
          $value = $view_copy->render_field($field_name, $index);
          $oids[$key] = empty($value) ? (empty($key) ? t('empty') : $key): $value; 
      }
      
      // Check if limited exceeded, if so, this does not look like a good candidate.
      if(count($oids) == $max_items){
        drupal_set_message(t('Selective filter {$field} has limited the amount of total results. Please, review you query configuration.'));
      } 
      
      self::$results[$signature] = $oids;
      $view_copy->destroy();
    }
    return self::$results[$signature];
  }
}

