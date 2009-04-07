<?php
// $Id$

/**
 * @file
 * Theme functions.
 */

/**
 * Theme function
 */
function theme_viewfield_formatter($element) {
  _viewfield_load_view_info($element);
  $node = $element['#node'];
  if ($view_info = explode('|', $element['#item']['vname'])) {
    $view_name = $view_info[0];
    $display = $view_info[1];
  }
  else {
    $view_name = $element['#item']['vname'];
    $display = $element['#item']['display'];
  }
  $view_args = _viewfield_get_view_args($element['#item']['token_enabled'], $element['#item']['vargs'], $element['#node']);
  return _viewfield_get_view_content($view_name, $view_args, $display, $node);
}

/**
 * Load the view field data into the given $element if missing
 */
function _viewfield_load_view_info(&$element) {
  // Check if the node has been loaded.
  if (!isset($element['#item']['token_enabled'])) {
    static $fields;
    $param_key = $element['#type_name'] .'-'. $element['#field_name'];
    // Load field info.
    if (!isset($fields[$param_key])) {
      module_load_include('inc', 'content', 'includes/content.crud');
      $param = array('type_name' => $element['#type_name'], 'field_name' => $element['#field_name']);
      $fields[$param_key] = content_field_instance_read($param);
    }
    $field = $fields[$param_key][0];
    if ($element['#item']['vname'] == VIEWFIELD_DEFAULT_VNAME) {
      // We are in the default land here.
      $element['#item'] = _viewfield_get_super_defaults($field);
    }
    $element['#item']['token_enabled'] = _viewfield_token_enabled($field);
  }
}

/**
 * Perform argument replacement
 */
function _viewfield_get_view_args($token_enabled, $vargs, $node) {
  $args = array();
  // Prevent token_replace() from running this function a second time
  // before it completes the first time.
  static $tokens = TRUE;
  if ($tokens && !empty($vargs)) {
    $pos = 0;
    while ($pos < strlen($vargs)) {
      $found = FALSE;
      // If string starts with a quote, start after quote and get everything
      // before next quote.
      if (strpos($vargs, '"', $pos) === $pos) {
        if (($quote = strpos($vargs, '"', ++$pos)) !== FALSE) {
          // Skip pairs of quotes.
          while (!(($ql = strspn($vargs, '"', $quote)) & 1)) {
            $quote = strpos($vargs, '"', $quote + $ql);
          }
          $args[] = str_replace('""', '"', substr($vargs, $pos, $quote + $ql - $pos - 1));
          $pos = $quote + $ql + 1;
          $found = TRUE;
        }
      }
      elseif (($comma = strpos($vargs, ',', $pos)) !== FALSE) {
        // Otherwise, get everything before next comma.
        $args[] = substr($vargs, $pos, $comma - $pos);
        // Skip to after comma and repeat
        $pos = $comma + 1;
        $found = TRUE;
      }
      if (!$found) {
        $args[] = substr($vargs, $pos);
        $pos = strlen($vargs);
      }
    }
    if ($token_enabled) {
      $tokens = FALSE;
      // If the view field is being loaded as a "view field" of "view row",
      // instead of a simple "node field", the node object is not fully populated:
      // we need a full node to perform a correct replacement.
      $node_values = $node;
      if ($node->build_mode == 'views') {
        $node_values = node_load($node->nid);
      }
      $args = token_replace($args, 'node', $node_values);
      $tokens = TRUE;
    }
    // For backwards compatibility, we scan for %nid, etc.
    global $user;
    foreach ($args as $key => $a) {
      $args[$key] = strtr($a, array('%nid' => $node->nid, '%author' => $node->uid, '%viewer' => $user->uid));
    }
  }
  return $args;
}

/**
 * Return a themed view avoiding viewfield recursion.
 */
function _viewfield_get_view_content($view_name, $view_args, $display, $node) {
  global $_viewfield_stack;
  global $user;
  $output = '';
  // For safety's sake, we can only display 2 levels of viewfields.
  if (!empty($view_name) && count($_viewfield_stack) <= 2) {
    // Need to prevent recursive views and node building, but don't need to do
    // it on new node previews.
    if ($node->nid) {
      _viewfield_nodestack_push($node->nid);
    }
    $view = views_get_view($view_name);
    if (!empty($view)) {
      $view->set_display($display);
      $view->set_arguments($view_args);
      if ($view->access(array($display), $user)) {
        $view->execute_display();
        $output .= $view->render();
      }
    }
    // This node is "safe" again.
    if ($node->nid) {
      _viewfield_nodestack_pop();
    }
  }
  return $output;
}

/**
 * Function for adding a node ID to the global stack of node IDs. This prevents
 * us from recursively building a node, with a view, with the node, with the
 * view...
 */
function _viewfield_nodestack_push($nid) {
  global $_viewfield_stack;
  if (!isset($_viewfield_stack)) {
    $_viewfield_stack = array();
  }
  $_viewfield_stack[] = $nid;
}

/**
 * Function for removing a node ID from the global stack of node IDs when there
 * is no longer a danger of building a node, with a view, with the node, with
 * the view...
 */
function _viewfield_nodestack_pop() {
  global $_viewfield_stack;
  return array_pop($_viewfield_stack);
}

function theme_viewfield_select($element) {
  if (!empty($element['#children'])) {
    $field = $element['#field_info'][$element['#field_name']];
    if ($field['multiple'] && $element['#delta'] == 0) {
      // This is needed only for multiple viewfields.
      drupal_add_css(drupal_get_path('module', 'viewfield') .'/theme/viewfield.css');
    }
    return '<div class="viewfield-select">'. $element['#children'] .'</div>';
  }
}