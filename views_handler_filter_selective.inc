<?php
/**
 * @file
 * Views Filter Selective Handler Overrides.
 */

/**
 * {@inheritdoc}
 */
class views_handler_filter_selective extends views_handler_filter_in_operator {
  protected static $results;

  /**
   * Override set_default_options.
   */
  public function set_default_options() {
    parent::set_default_options();
    // This filter showld always be exposed!
    $this->options['exposed'] = TRUE;
    // Do not allow to reduce options, makes no sense.
    $this->options['expose']['reduce'] = FALSE;
  }

  /**
   * Override get_value_options: retrieve available values.
   */
  public function get_value_options() {
    $this->value_options = array();
    if (empty($this->view->selective_oids) && !empty($this->view->inited)) {
      $oids = $this->get_oids($this->definition['field_base'], $this->real_field, $this->field, $this->table, $this->table_alias, $this->options['expose']['identifier'], $this->options['ui_name']);
      $this->value_options = $oids;
      // TODO: Omit null values in result: they are improperly handled.
      // When constructing the query.
      $this->value_options = array_diff_key($this->value_options, array("" => NULL));
    }
  }

  /**
   * Override validate.
   */
  public function validate() {
    $errors = parent::validate();
    return $errors;
  }

  /**
   * Override value_form.
   */
  public function value_form(&$form, &$form_state) {
    parent::value_form($form, $form_state);
    // Avoid the 'illegal values' Form API error.
    $form['value']['#validated'] = TRUE;
  }

  /**
   * Override options_form.
   */
  public function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    // Filter should always be exposed, show warning.
    array_unshift($form['expose_button'], array(
      'warning' => array(
        '#type' => 'markup',
        '#markup' => '<div class="messages warning">' . t('This filter is always exposed to users.') . '</div>',
      )));
    // Remove option to unexpose filter. Tried to disable, but did not work.
    $form['expose_button']['checkbox']['checkbox']['#type'] = 'hidden';
    unset($form['expose_button']['button']);
    unset($form['expose_button']['markup']);
    // Change "All Values" to "Available at runtime".
    $form['value']['#options']['all'] = t('Actual values will be available at run-time');
    // Do not allow to check "all values".
    $form['value']['#attributes']['disabled'] = 'disabled';
    // Cannot group without values.
    unset($form['group_button']);
  }

  /**
   * Override expose_form.
   */
  public function expose_form(&$form, &$form_state) {
    parent::expose_form($form, $form_state);
    // Remove reduce resultset interface.
    unset($form['expose']['reduce']);
    // TODO: Populated somewhere through AJAX, I could not find it....
    // Provide default value for filter name.
    if (empty($form['expose']['identifier']['#default_value'])) {
      $form['expose']['identifier']['#default_value'] = $this->options['field'];
    }
    if (empty($form['expose']['label']['#default_value'])) {
      $form['expose']['label']['#default_value'] = $this->definition['title'];
    }
    if (empty($form['ui_name']['#default_value'])) {
      $form['ui_name']['#default_value'] = $this->definition['title'];
    }
  }

  /**
   * Override query.
   */
  public function query() {
    // Decode the values to restore special chars.
    $this->value = array_map('urldecode', $this->value);
    parent::query();
  }

  /**
   * Get list of options for current view, only at runtime.
   */
  protected function get_oids($base_field, $real_field, $field, $table, $table_alias, $exposed_identifier, $ui_name) {
    $exposed_input = isset($this->view->exposed_input) ? $this->view->exposed_input : array();
    $exposed_input = array_merge(array(), $exposed_input);
    unset($exposed_input[$exposed_identifier]);
    $signature = md5(serialize(array(
      'name' => $this->view->name,
      'args' => $this->view->args,
      'input' => $exposed_input,
      'base_field' => $base_field,
      'real_field' => $real_field,
      'field' => $field,
      'table' => $table,
      'ui_name' => $ui_name,
    )));
    if (empty(self::$results[$signature])) {
      $max_items = 100;

      // Clone the view (so it works while editting) and get all results.
      $view_copy = $this->view->clone_view();
      if (!$view_copy) {
        return NULL;
      }
      $view_copy->selective_oids = TRUE;
      $view_copy->set_exposed_input($exposed_input);
      $view_copy->set_arguments($this->view->args);

      // Mess up with the field used for distinct have thousands of elements.
      // Limit result set to 100: anything above is not user friendly at all.
      $view_copy->set_items_per_page($max_items);

      // Remove paging, items per page and page number from context.
      if (isset($_GET['items_per_page'])) {
        $items_per_page = $_GET['items_per_page'];
        unset($_GET['items_per_page']);
      }
      if (isset($_GET[$exposed_identifier])) {
        $exposed_filter = $_GET[$exposed_identifier];
        unset($_GET[$exposed_identifier]);
      }
      if (isset($_GET['page'])) {
        $exposed_page = $_GET['page'];
        unset($_GET['page']);
      }

      // Remove "our" field from exposed input.
      unset($view_copy->exposed_input[$field]);

      // Manipulate display + default: don't know if fields are overriden.
      $display = $view_copy->display[$this->view->current_display];
      $display_default = $view_copy->display['default'];

      // Remove any exposed form configuration. This showed up with BEF module!
      unset($display->display_options['exposed_form']);
      unset($display_default->display_options['exposed_form']);

      // If we are using fields from default or current display.
      if (isset($display->display_options['fields'])) {
        $display_options_fields = &$display->display_options['fields'];
      }
      else {
        $display_options_fields = &$display_default->display_options['fields'];
      }

      // Remove all fields but the aggregate one.
      foreach ($display_options_fields as $key => $value) {
        if (((isset($value['ui_name']) && $value['ui_name']) != $ui_name) || ($value['field'] != $base_field)) {
          unset($display_options_fields[$key]);
        }
      }

      // Check to see if the user remembered to add the field.
      if (empty($display_options_fields)) {
        drupal_set_message(t('Selective query filter must have corresponding field added to view with Administrative Name set to "@name" and Base Type "@type"',
          array(
            '@name' => $ui_name,
            '@type' => $base_field)),
            'error');
        return array();
      }

      // Get ID of field that will be used for rendering.
      $display_field = reset($display_options_fields);

      // Get field Id.
      $display_field_id = $display_field['id'];

      // Check that relationships are coherent between Field and Filter.
      // Only check if both are not empty.
      if(!(empty($display_field['relationship']) && empty($this->options['relationship']))) {
        if (empty($display_field['relationship']) || empty($this->options['relationship']) || ($display_field['relationship'] != $this->options['relationship'])) {
          drupal_set_message(t('Selective filter "@name (@type)": relationship of field and filter must match.',
           array(
            '@name' => $ui_name,
             '@type' => $base_field)),
             'error');
          return array();
        }
      }

      // If main field is excluded form presentation, bring it back.
      // Set group type for handler to populate database relationships in query.
      $display_field['exclude'] = 0;
      $display_field['group_type'] = 'group';

      // Remove all sorting.
      unset($display->display_options['sorts']);
      unset($display_default->display_options['sorts']);

      // Turn this into an aggregate query.
      $display->display_options['group_by'] = 1;
      $display->handler->options['group_by'] = 1;

      $display_default->display_options['group_by'] = 1;
      $display_default->handler->options['group_by'] = 1;

      // Aggregate is incompatible with distinct and pure distinct.
      unset($display_default->display_options['query']['options']['distinct']);
      unset($display_default->display_options['query']['options']['pure_distinct']);

      unset($display->display_options['query']['options']['distinct']);
      unset($display->display_options['query']['options']['pure_distinct']);

      // Run View.
      $view_copy->execute($this->view->current_display);

      // Reset context parameters for real View.
      if (isset($items_per_page)) {
        $_GET['items_per_page'] = $items_per_page;
      }
      if (isset($exposed_filter)) {
        $_GET[$exposed_identifier] = $exposed_filter;
      }
      if (isset($exposed_page)) {
        $_GET['page'] = $exposed_page;
      }

      // Get Handler after execution.
      $display_field_handler = $view_copy->field[$display_field_id];

      // Create array of objects for selector.
      $oids = array();
      $field_alias = isset($display_field_handler->aliases[$display_field_handler->real_field]) ? $display_field_handler->aliases[$display_field_handler->real_field] : $display_field_handler->table_alias . '_' . $display_field_handler->real_field;
      foreach ($view_copy->result as $index => $row) {
        // Using: $key = $display_field_handler->get_value($row); should be more robust.
        // But returned values are sometimes nested arrays, and we need single string values.
        // For the filters.
        $key = $display_field_handler->get_value($row);
        if (is_array($key)) {
          $key = $row->{$field_alias};
        }
        $value = $view_copy->render_field($display_field_id, $index);
        $oids[$key] = empty($value) ? t('Empty (@key)', array('@key' => empty($key) ? json_encode($key) : $key)) : $value;
      }

      // Sort values alphabetically.
      asort($oids);

      // If limit exceeded this field is not good for being "selective".
      if (count($oids) == $max_items) {
        drupal_set_message(t('Selective filter "@field" has limited the amount of total results. Please, review you query configuration.', array('@field' => $ui_name)), 'warning');
      }

      // TODO: unset caching options, due to unfixed old bug:
      // https://drupal.org/node/1055616. If people are smart
      // they will not enable caching on view with exposed
      // filter until bug fixed.

      self::$results[$signature] = $oids;
      $view_copy->destroy();
    }
    return self::$results[$signature];
  }
}
